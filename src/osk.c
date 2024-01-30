#include <SDL2/SDL.h>
#include <sysutil/osk.h>
#include <sys/memory.h>
#include <sysutil/sysutil.h>

#include "unicode.h"
#include "assert.h"
#include "stdint.h"
#include "types.h"
#include "osk.h"

// Set to true while the OSK is open, prevents the OSK from being opened twice by accident
bool osk_running = false;
sys_mem_container_t containerid;
oskCallbackReturnParam outputParam = {0};
uint8_t utf8_output[OSK_TEXT_BUFFER_LENGTH] = {0};
bool was_good = false;

void sysutil_exit_callback(u64 status, u64 param, void *usrdata)
{
    switch (status)
    {
    case SYSUTIL_EXIT_GAME:
        break;
    case SYSUTIL_DRAW_BEGIN:
    case SYSUTIL_DRAW_END:
        break;
    case SYSUTIL_OSK_LOADED:
        printf("OSK loaded\n");
        break;
    case SYSUTIL_OSK_INPUT_CANCELED:
        printf("OSK input canceled\n");
        oskAbort();
        // fall-through
    case SYSUTIL_OSK_DONE:
        if (status == SYSUTIL_OSK_DONE)
        {
            printf("OSK done\n");
        }

        oskUnloadAsync(&outputParam);

        state_t *state = (state_t *)usrdata;

        was_good = outputParam.res == OSK_OK;
        if (outputParam.res == OSK_OK)
        {
            printf("OSK result OK\n");
        }
        else
        {
            printf("OKS result: %d\n", outputParam.res);
        }

        utf16_to_utf8(state->osk_buffer, utf8_output);

        break;
    case SYSUTIL_OSK_UNLOADED:
        printf("OSK unloaded\n");
        osk_running = false;
        break;
    default:
        break;
    }
}

void osk_setup(state_t *state)
{
    oskSetInitialInputDevice(OSK_DEVICE_PAD);
    oskSetKeyLayoutOption(OSK_FULLKEY_PANEL);
    oskSetLayoutMode(OSK_LAYOUTMODE_HORIZONTAL_ALIGN_CENTER | OSK_LAYOUTMODE_VERTICAL_ALIGN_BOTTOM);

    ASSERT_ZERO(sysMemContainerCreate(&containerid, 4 * 1024 * 1024), "Failed to create OSK memory container");

    outputParam.res = OSK_OK;
    outputParam.len = OSK_TEXT_BUFFER_LENGTH - 1;
    outputParam.str = state->osk_buffer;

    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, sysutil_exit_callback, state);
}

void osk_open(uint16_t *title, uint16_t *initial_text)
{
    oskInputFieldInfo inputFieldInfo = {0};
    inputFieldInfo.message = title;
    inputFieldInfo.startText = initial_text;
    inputFieldInfo.maxLength = OSK_TEXT_BUFFER_LENGTH - 1;

    oskParam parameters = {0};
    parameters.allowedPanels = OSK_PANEL_TYPE_DEFAULT;
    parameters.firstViewPanel = OSK_PANEL_TYPE_DEFAULT;
    parameters.controlPoint = (oskPoint){0, 0};
    parameters.prohibitFlags = OSK_PROHIBIT_RETURN; // This will disable entering a new line

    ASSERT_ZERO(oskLoadAsync(containerid, &parameters, &inputFieldInfo), "Failed to load OSK");

    osk_running = true;
}

bool is_osk_running()
{
    return osk_running;
}

char *get_utf8_output()
{
    if (!was_good)
        return NULL;

    return (char *)utf8_output;
}