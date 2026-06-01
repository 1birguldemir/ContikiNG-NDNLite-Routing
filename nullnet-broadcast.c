/*
 * Copyright (c) 2017, RISE SICS.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         NullNet broadcast example
 * \author
*         Simon Duquennoy <simon.duquennoy@ri.se>
 *
 */

#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "sys/node-id.h"
#include <string.h>
#include <stdio.h> /* For printf() */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)

#if MAC_CONF_WITH_TSCH
#include "net/mac/tsch/tsch.h"
static linkaddr_t coordinator_addr =  {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
#endif /* MAC_CONF_WITH_TSCH */

/*Kendi özel veri paketimiz*/
typedef struct 
{
  uint16_t src_id; /* Source node ID */
  uint16_t dest_id; /* Destination node ID */
  unsigned count; /* sayaç verisi */
}custom_packet_t;

static custom_packet_t packet_to_send; /* Gönderilecek veri paketi */


/*---------------------------------------------------------------------------*/
PROCESS(nullnet_example_process, "NullNet broadcast example");
AUTOSTART_PROCESSES(&nullnet_example_process);

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
uint8_t my_id = linkaddr_node_addr.u8[0]; /* Mote ID'sini al (linkaddr_node_addr'un ilk byte'ı) */

  if(len == sizeof(custom_packet_t)) {
    custom_packet_t received_packet;
    memcpy(&received_packet, data, sizeof(received_packet));/* Gelen veriyi kendi özel paket yapımıza kopyala */
    LOG_INFO("Kaynak: Mote %u , Hedef: Mote %u, Sayac: %u ", received_packet.src_id, received_packet.dest_id, received_packet.count); // Gelen veriyi logla
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");

    if(received_packet.dest_id == my_id) { /* Eğer gelen paket benim içinse */
      LOG_INFO("Bu paket bana ait!\n");
    }
    else if(my_id==2){
      LOG_INFO("Mote 2 : Paket Mote %u için kopruleniyor\n", received_packet.dest_id);
      
      memcpy(nullnet_buf, &received_packet, sizeof(received_packet)); /* Gelen veriyi tekrar nullnet bufferına kopyala */
    nullnet_len = sizeof(received_packet);
    NETSTACK_NETWORK.output(NULL); /* Paketi tekrar gönder (kopyalayarak) */
  }
    }

}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nullnet_example_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned count = 0;

  PROCESS_BEGIN();

#if MAC_CONF_WITH_TSCH
  tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
#endif /* MAC_CONF_WITH_TSCH */

  /* Initialize NullNet */
  nullnet_buf = (uint8_t *)&packet_to_send;
  nullnet_len = sizeof(packet_to_send);
  nullnet_set_input_callback(input_callback);

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    uint8_t my_id = linkaddr_node_addr.u8[0]; /* Mote ID'sini al (linkaddr_node_addr'un ilk byte'ı) */

    /* Gönderilecek paketi hazırla */
    if(my_id==3){
      packet_to_send.src_id = 3; /* Kaynak ID'si olarak 3 */
      packet_to_send.dest_id = 1; /* Hedef ID'si olarak Mote 1'i belirle */
      packet_to_send.count = count; /* Sayaç verisini güncelle */
    
    LOG_INFO("Mote 3: Mote 1'ye veri gönderiyor: %u ", count);
    LOG_INFO_LLADDR(NULL);
    LOG_INFO_("\n");
    
    /* Paketi nullnet bufferına kopyala ve gönder */
    memcpy(nullnet_buf, &packet_to_send, sizeof(packet_to_send));
    nullnet_len = sizeof(packet_to_send);

    NETSTACK_NETWORK.output(NULL);
    count++;
  }
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
