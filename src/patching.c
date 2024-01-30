#include <tre.h>
#include <stdio.h>
#include <unistd.h>

#include "assert.h"
#include "types.h"
#include "scetool.h"
#include "copyfile.h"
#include "license.h"
#include "digest.h"

void patch_game(void *arg)
{
    state_t *state = (state_t *)arg;

    // Re-Init libscetool
    ASSERT_ZERO(libscetool_init(), "Unable to initialize libscetool");

    // Get the path to the EBOOT.BIN
    char eboot_path[256] = {0};
    snprintf(eboot_path, 256, "%s/USRDIR/EBOOT.BIN", state->selected_game->path);

    // Get the path to the EBOOT.BIN.BAK
    char eboot_bak_path[256] = {0};
    snprintf(eboot_bak_path, 256, "%s/USRDIR/EBOOT.BIN.BAK", state->selected_game->path);

    // Get the path to the patched EBOOT.BIN.PATCHED
    char patched_eboot_path[256] = {0};
    snprintf(patched_eboot_path, 256, "%s/USRDIR/EBOOT.BIN.PATCHED", state->selected_game->path);

    SDL_Log("Backing up EBOOT.BIN if it doesn't exist");

    // If the backup EBOOT does not exist
    if (access(eboot_bak_path, F_OK) != 0)
    {
        // Set the state to backing up
        MUTEX_SCOPE(
            state->patching_info.mutex,
            {
                state->patching_info.state = PATCHING_STATE_BACKING_UP;
            });

        // Copy the EBOOT.BIN to EBOOT.BIN.BAK
        ASSERT_ZERO(copy_file(eboot_bak_path, eboot_path), "Unable to copy EBOOT.BIN to EBOOT.BIN.BAK");
    }

    SDL_Log("Set encrypt options");

    // If this is an NPDRM game, set the NPDRM encrypt options
    if (state->selected_game->title_id[0] == 'N')
        set_npdrm_encrypt_options();
    // Otherwise, set the disc encrypt options
    else
        set_disc_encrypt_options();

    // Set the state to decrypting
    MUTEX_SCOPE(
        state->patching_info.mutex,
        {
            state->patching_info.state = PATCHING_STATE_DECRYPTING;
        });

    SDL_Log("Setting IDPS key");

    set_idps_key(state->idps);

    SDL_Log("Getting content id");

    // Get the content id
    char *content_id = get_content_id(eboot_bak_path);

    // If the content id is NULL
    if (content_id == NULL)
    {
        // Set the state to error
        MUTEX_SCOPE(
            state->patching_info.mutex,
            {
                state->patching_info.state = PATCHING_STATE_ERROR;
                state->patching_info.is_running = false;
                state->patching_info.last_error = "Unable to get content id of executable.";
            });

        return;
    }

    SDL_Log("Content id: %.*s", 0x30, content_id);

    SDL_Log("Setting content id");

    set_npdrm_content_id(content_id);

    // Get a temp path for the decrypted EBOOT.BIN
    char eboot_decrypted_path[256] = {0};
    snprintf(eboot_decrypted_path, 256, "%s/USRDIR/EBOOT.BIN.DEC", state->selected_game->path);

    SDL_Log("Finding license");

    // Find the license
    char *license_path = find_license_from_all_users(content_id);

    // If the license is NULL
    if (license_path == NULL)
    {
        // Set the state to error
        MUTEX_SCOPE(
            state->patching_info.mutex,
            {
                state->patching_info.state = PATCHING_STATE_ERROR;
                state->patching_info.is_running = false;
                state->patching_info.last_error = "Unable to find license.";
            });

        return;
    }

    SDL_Log("Setting license paths");

    set_rif_file_path(license_path);
    rap_set_directory(license_path);

    SDL_Log("Decrypting");

    // Decrypt the EBOOT.BIN.BAK
    // The reason we always decrypt the EBOOT.BIN.BAK is because the EBOOT.BIN might have its digest patched.
    frontend_decrypt(eboot_bak_path, eboot_decrypted_path);

    SDL_Log("Searching");

    // Set the state to decrypting, since now we are searching for patchable elements in the EBOOT.BIN.DEC
    MUTEX_SCOPE(
        state->patching_info.mutex,
        {
            state->patching_info.state = PATCHING_STATE_SEARCHING;
        });

    // Open the decrypted EBOOT.BIN
    FILE *eboot_decrypted = fopen(eboot_decrypted_path, "rb");
    ASSERT_NONZERO(eboot_decrypted, "Unable to open decrypted EBOOT.BIN");

    // Get the size of the decrypted EBOOT.BIN
    fseek(eboot_decrypted, 0, SEEK_END);
    size_t eboot_decrypted_size = ftell(eboot_decrypted);
    fseek(eboot_decrypted, 0, SEEK_SET);

    // Allocate memory for the decrypted EBOOT.BIN
    uint8_t *eboot_decrypted_data = (uint8_t *)malloc(eboot_decrypted_size);
    ASSERT_NONZERO(eboot_decrypted_data, "Unable to allocate memory for decrypted EBOOT.BIN");

    while (fread(eboot_decrypted_data, sizeof(uint8_t), eboot_decrypted_size, eboot_decrypted) > 0)
    {
        SDL_Log("Read %d bytes", eboot_decrypted_size);
    }

    // Close the decrypted EBOOT.BIN
    ASSERT_ZERO(fclose(eboot_decrypted), "Unable to close decrypted EBOOT.BIN");

    char *url_regex_str = "^https?[^\\x00]//([0-9a-zA-Z.:].*)/?([0-9a-zA-Z_]*)$";

    regex_t url_regex;
    // Compile the URL regex
    ASSERT_ZERO(tre_regncomp(&url_regex, url_regex_str, strlen(url_regex_str), REG_EXTENDED), "Unable to compile url regex");

    // Iterate over ever 4 byte chunk in the decrypted EBOOT.BIN
    for (size_t i = 0; i < eboot_decrypted_size; i += 4)
    {
        char *str = (char *)eboot_decrypted_data + i;

        if (memcmp(eboot_decrypted_data + i, "http", 4) == 0)
        {
            SDL_Log("Found URL at address %x, %s", i, str);

            // find a match
            regmatch_t match[1];
            int ret = tre_regexec(&url_regex, str, 1, match, 0);

            if (ret == REG_NOMATCH)
            {
                continue;
            }
            else if (ret != 0)
            {
                char err_str[1024] = {0};
                tre_regerror(ret, &url_regex, err_str, 1024);
                SDL_Log("Matching url failed for some reason! err: %s", err_str);
                exit(1);
            }
            else
            {
                // If there was a match
                if (match[0].rm_so != -1)
                {
                    // Ignore format strings
                    if (strstr(str, "\%") != NULL)
                        continue;

                    // Count null bytes after str until next non-null byte
                    int null_bytes = 0;
                    for (int i = strlen(str); str[i] == '\0'; i++)
                    {
                        null_bytes++;
                    }

                    if (strlen(state->selected_server->url) > (strlen(str) + null_bytes - 1))
                    {
                        // Set the state to error
                        MUTEX_SCOPE(
                            state->patching_info.mutex,
                            {
                                state->patching_info.state = PATCHING_STATE_ERROR;
                                state->patching_info.is_running = false;
                                state->patching_info.last_error = "URL too long to fit in EBOOT.";
                            });

                        // Get outta here
                        goto out;
                    }

                    SDL_Log("Found valid URL at address %x, %s. Patching...", i, str);

                    // Null out the original string
                    memset(str, '\0', strlen(str));

                    // Copy the new URL in
                    strcpy(str, state->selected_server->url);
                }
            }
        }
        // If we find the word "cookie", then we know that the digest key is somewhere near it
        else if (memcmp(eboot_decrypted_data + i, "cookie", 7) == 0)
        {
            SDL_Log("Found cookie at address %x, %s", i, str);

            const int digest_key_range = 1000;

            size_t start = i - digest_key_range;
            size_t end = i + digest_key_range;

            for (size_t j = start; j < end; j += 1)
            {
                char *search_str = (char *)eboot_decrypted_data + j;

                int len = strlen(search_str);
                if (len != 18)
                {
                    j += len;
                    continue;
                }

                if (valid_digest(search_str))
                {
                    SDL_Log("Found digest at address %x, %s. Patching...", j, search_str);

                    // Copy the new digest in
                    strcpy(search_str, "CustomServerDigest");
                }
            }
        }
    }

    // Write out the patched EBOOT.ELF to EBOOT.BIN.PATCHED
    FILE *eboot_patched = fopen(patched_eboot_path, "wb");
    ASSERT_NONZERO(eboot_patched, "Unable to open patched EBOOT.BIN");

    SDL_Log("Writing patched EBOOT.BIN.PATCHED");

    int to_write = eboot_decrypted_size;
    while (to_write > 0)
    {
        int written = fwrite(eboot_decrypted_data + (eboot_decrypted_size - to_write), sizeof(uint8_t), to_write, eboot_patched);
        if (written <= 0)
        {
            SDL_Log("Unable to write to patched EBOOT.BIN");
            exit(1);
        }

        to_write -= written;
    }

    // Close the patched EBOOT.BIN
    ASSERT_ZERO(fclose(eboot_patched), "Unable to close patched EBOOT.BIN");

    free(eboot_decrypted_data);

    SDL_Log("Encrypting");

    // Set the state to done
    MUTEX_SCOPE(
        state->patching_info.mutex,
        {
            state->patching_info.state = PATCHING_STATE_ENCRYPTING;
        });

    // Encrypt the patched EBOOT.BIN
    frontend_encrypt(patched_eboot_path, eboot_path);

    // Set the state to done
    MUTEX_SCOPE(
        state->patching_info.mutex,
        {
            state->patching_info.state = PATCHING_STATE_DONE;
            state->patching_info.is_running = false;
            state->patching_info.last_error = NULL;
        });

out:
    tre_regfree(&url_regex);

    return;
}