#include <stdint.h>

int libscetool_init();

void frontend_print_infos(char *file_path);
void frontend_decrypt(char *file_path, char *out_path);
void frontend_encrypt(char *file_path, char *out_path);
void rap_set_directory(char *dir_path);
void set_idps_key(char *key);
void set_act_dat_file_path(char *file_path);
void set_rif_file_path(char *dir_path);
void set_disc_encrypt_options();
void set_npdrm_encrypt_options();
void set_npdrm_content_id(char *content_id);
char *get_content_id(char *file_path);