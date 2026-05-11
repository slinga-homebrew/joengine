/*
** Jo Sega Saturn Engine
** Copyright (c) 2012-2020, Johannes Fetz (johannesfetz@gmail.com)
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the Johannes Fetz nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AN
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL Johannes Fetz BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <jo/modem.h>
#include <jo/jo.h>
#include <string.h>

/*============================================================================
 * Configuration
 *============================================================================*/

#define CONNECT_DIAL_NUMBER   "199407"
#define CONNECT_DIAL_TIMEOUT  180000000  /* ~60 seconds at 28.6MHz */

/*============================================================================
   * Connection State Machine
   *
   * Each stage sets the status text, returns to the main loop (so the draw
   * callback renders the message), then the NEXT frame executes the blocking
   * modem call and advances to the following stage.
   *============================================================================*/
  
typedef enum {
    CONNECT_STAGE_INIT = 0,
    CONNECT_STAGE_SHOW_PROBE,    /* Display "Probing modem..." */
    CONNECT_STAGE_PROBING,       /* Execute modem_probe() */
    CONNECT_STAGE_SHOW_INIT,     /* Display "Initializing modem..." */
    CONNECT_STAGE_MODEM_INIT,    /* Execute modem_init() */
    CONNECT_STAGE_SHOW_DIAL,     /* Display "Dialing server..." */
    CONNECT_STAGE_DIALING,       /* Execute modem_dial() */
    CONNECT_STAGE_CONNECTED,     /* Success - transition to lobby */
    CONNECT_STAGE_FAILED,        /* Failure - wait then return to title */
} connect_stage_t;

// heartbeat message to send periodically
const char g_heartbeat[] = "LOOC si sihT ";
unsigned int g_heartbeat_counter = 0;

unsigned char g_recv_buf[16] = {0};
unsigned char g_send_buf[16] = {0};

unsigned int g_frame = 0;
unsigned int g_netlink_sprite_id = 0;
unsigned int g_x_sprite_id = 0;

bool g_modem_detected = false;
connect_stage_t g_connect_stage = CONNECT_STAGE_INIT;
saturn_uart16550_t g_uart = {0};
char* g_connect_msg = NULL;

void my_connecting_update(void);
void my_modem_recv(void);
void my_modem_send(void);
void my_gamepad(void);
void my_draw(void);
void my_frame_counter(void);
void prepend_array(unsigned char* array, unsigned int array_size, unsigned char val);
char convert_button_to_char(void);

void			jo_main(void)
{
    jo_core_init(JO_COLOR_Black);

    g_modem_detected = modem_is_present();
    if(g_modem_detected)
    {
        modem_get_uart(&g_uart);
    }

    jo_core_add_callback(my_draw);
    jo_core_add_callback(my_frame_counter);	
    jo_core_add_callback(my_gamepad);
    jo_core_add_callback(my_connecting_update);
    jo_core_add_callback(my_modem_recv);
    jo_core_add_callback(my_modem_send);

    g_netlink_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "MODEM.TGA", JO_COLOR_Green);
    g_x_sprite_id = jo_sprite_add_tga(JO_ROOT_DIR, "X.TGA", JO_COLOR_Black);

    g_connect_stage = CONNECT_STAGE_INIT;

    jo_core_run();
}

// drawing callback
void			my_draw(void)
{
    unsigned int i = 0;

    jo_printf(11, 2, "Dial-Up Modem Demo");	

    jo_sprite_change_sprite_scale(1.5);	
    jo_sprite_draw3D(g_netlink_sprite_id, 0,-40, 500);

    // draw an X if the modem is not present
    if(g_modem_detected == false)
    {
        jo_sprite_draw3D(g_x_sprite_id, 0,-40, 500);
    }
    jo_sprite_restore_sprite_scale();

    jo_printf(1, 18, "Status: (%d) %-25s", g_connect_stage, g_connect_msg);
    
    if(g_connect_stage != CONNECT_STAGE_CONNECTED)
    {
        return;
    }

    jo_printf(1, 20, "Send:");
    for(i = 0; i < sizeof(g_send_buf); i++)
    {
        jo_printf(7 + (i * 2), 20, "%c", g_send_buf[i]);
    }

    jo_printf(1, 22, "Recv:");
    for(i = 0; i < sizeof(g_recv_buf); i++)
    {
        jo_printf(7 + (i * 2), 22, "%c", g_recv_buf[i]);
    }

    jo_printf(1, 24, "Press buttons to send data");
    jo_printf(1, 25, "Sends \"This is COOL\" char periodically");
}

void my_connecting_update(void)
{
    modem_result_t result;    

    switch (g_connect_stage) {

    case CONNECT_STAGE_INIT:
        if (!g_modem_detected) {
            g_connect_msg = "No modem";
            //dnet_log("No NetLink modem detected");
            g_connect_stage = CONNECT_STAGE_FAILED;
            return;
        }
        /* Advance to show probe message next frame */
        g_connect_stage = CONNECT_STAGE_SHOW_PROBE;
        break;

    case CONNECT_STAGE_SHOW_PROBE:
        /* Set message and let it render for one frame */
        g_connect_msg = "Probing modem...";
        //dnet_log("Probing modem...");
        g_connect_stage = CONNECT_STAGE_PROBING;
        break;

    case CONNECT_STAGE_PROBING:
        /* Force frame render right before blocking call */
        slSynch();

        if (modem_probe(&g_uart) != MODEM_OK) {
            g_connect_msg = "No modem response";
            //dnet_log("No modem response");
            g_connect_stage = CONNECT_STAGE_FAILED;
            return;
        }
        //dnet_log("Modem detected");
        g_connect_stage = CONNECT_STAGE_SHOW_INIT;
        break;

    case CONNECT_STAGE_SHOW_INIT:
        g_connect_msg = "Initializing modem...";
        //dnet_log("Initializing modem...");
        g_connect_stage = CONNECT_STAGE_MODEM_INIT;
        break;

    case CONNECT_STAGE_MODEM_INIT:
        slSynch();

        if (modem_init(&g_uart) != MODEM_OK) {
            g_connect_msg = "Modem init failed";
            //dnet_log("Modem init failed");
            g_connect_stage = CONNECT_STAGE_FAILED;
            return;
        }
        //dnet_log("Modem ready");
        g_connect_stage = CONNECT_STAGE_SHOW_DIAL;
        break;

    case CONNECT_STAGE_SHOW_DIAL:
        g_connect_msg = "Dialing server...";
        //dnet_log("Dialing " CONNECT_DIAL_NUMBER "...");
        g_connect_stage = CONNECT_STAGE_DIALING;
        break;

    case CONNECT_STAGE_DIALING:
        slSynch();

        result = modem_dial(&g_uart, CONNECT_DIAL_NUMBER, CONNECT_DIAL_TIMEOUT);
        switch (result) {
        case MODEM_CONNECT:
            g_connect_msg = "Connected!";
            //dnet_log("Connection established!");
            modem_flush_input(&g_uart);
            g_connect_stage = CONNECT_STAGE_CONNECTED;
            break;
        case MODEM_NO_CARRIER:
            g_connect_msg = "NO CARRIER";
            //dnet_log("NO CARRIER - Check cable");
            g_connect_stage = CONNECT_STAGE_FAILED;
            break;
        case MODEM_BUSY:
            g_connect_msg = "LINE BUSY";
            //dnet_log("LINE BUSY - Try again");
            g_connect_stage = CONNECT_STAGE_FAILED;
            break;
        case MODEM_NO_DIALTONE:
            g_connect_msg = "NO DIALTONE";
            //dnet_log("NO DIALTONE - Check line");
            g_connect_stage = CONNECT_STAGE_FAILED;
            break;
        case MODEM_NO_ANSWER:
            g_connect_msg = "NO ANSWER";
            //dnet_log("NO ANSWER - Server down?");
            g_connect_stage = CONNECT_STAGE_FAILED;
            break;
        case MODEM_TIMEOUT_ERR:
            g_connect_msg = "TIMEOUT";
            //dnet_log("TIMEOUT - Server offline?");
            g_connect_stage = CONNECT_STAGE_FAILED;
            break;
        default:
            g_connect_msg = "Unknown error";
            //dnet_log("Dial failed");
            g_connect_stage = CONNECT_STAGE_FAILED;
            break;
        }
        break;

    case CONNECT_STAGE_CONNECTED:
        // msg connected
        break;

    case CONNECT_STAGE_FAILED:        
        break;
    }
}


// callback to check controller 1 input
// if controller input, send via modem
void			my_gamepad(void)
{
    unsigned char data = 0;
    int result = 0;

    if(g_connect_stage != CONNECT_STAGE_CONNECTED)
    {
        return;
    }

    if(!jo_is_input_available(0))
    {
        return;
    }

    data = convert_button_to_char();
    if(!data)
    {
        return;
    }
    
    // send a single byte over the modem
    result = modem_send_bytes(&g_uart, &data, sizeof(data));
    if(result != 1)
    {
        return;
    }    

    prepend_array(g_send_buf, sizeof(g_send_buf), data);
}

// send a heartbeat character over the modem link every 255 frames
// message = "This is COOL"
void			my_modem_send(void)
{
    unsigned char data = 0;
    int result = 0;

    if(g_connect_stage != CONNECT_STAGE_CONNECTED)
    {
        return;
    }

    if ((g_frame & 255) != 0)
    {
        return;
    }

    data = g_heartbeat[g_heartbeat_counter];
    g_heartbeat_counter++;

    if(g_heartbeat_counter >= sizeof(g_heartbeat))
    {
        g_heartbeat_counter = 0;
    }	

    // send a single byte over the modem
    result = modem_send_bytes(&g_uart, &data, sizeof(data));
    if(result != 1)
    {
        return;
    }

    prepend_array(g_send_buf, sizeof(g_send_buf), data);
}

// callback to check if modem data is available
void			my_modem_recv(void)
{
    unsigned char data = 0;
    int result = 0;

    if(g_connect_stage != CONNECT_STAGE_CONNECTED)
    {
        return;
    }

    // check if we have received data
    result = modem_recv_ready(&g_uart);
    if(result == false)
    {
        return;
    }

    data = modem_recv_byte(&g_uart);

    prepend_array(g_recv_buf, sizeof(g_recv_buf), data);
}

void			my_frame_counter(void)
{
    g_frame++;
}

void			prepend_array(unsigned char* array, unsigned int array_size, unsigned char val)
{
    memmove(array + 1, array, array_size -1);
    array[0] = val;
}

char			convert_button_to_char(void)
{
    if(jo_is_input_key_down(0, JO_KEY_RIGHT))
    {
        return 'r';
    }
    else if(jo_is_input_key_down(0, JO_KEY_LEFT))
    {
        return 'l';
    }
    else if(jo_is_input_key_down(0, JO_KEY_DOWN))
    {
        return 'd';
    }
    else if(jo_is_input_key_down(0, JO_KEY_UP))
    {
        return 'u';
    }
    else if(jo_is_input_key_down(0, JO_KEY_START))
    {
        return 'S';
    }
    else if(jo_is_input_key_down(0, JO_KEY_A))
    {
        return 'A';
    }
    else if(jo_is_input_key_down(0, JO_KEY_B))
    {
        return 'B';
    }
    else if(jo_is_input_key_down(0, JO_KEY_C))
    {
        return 'C';
    }
    else if(jo_is_input_key_down(0, JO_KEY_X))
    {
        return 'X';
    }
    else if(jo_is_input_key_down(0, JO_KEY_Y))
    {
        return 'Y';
    }
    else if(jo_is_input_key_down(0, JO_KEY_Z))
    {
        return 'Z';
    }
    else if(jo_is_input_key_down(0, JO_KEY_LEFT))
    {
        return 'L';
    }
    else if(jo_is_input_key_down(0, JO_KEY_RIGHT))
    {
        return 'R';
    }

    return 0;
}

/*
** END OF FILE
*/
