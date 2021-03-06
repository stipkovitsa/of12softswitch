/* Copyright (c) 2011, TrafficLab, Ericsson Research, Hungary
 * Copyright (c) 2012, CPqD, Brazil 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Ericsson Research nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#include <arpa/inet.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>
#include <netinet/in.h>
#include "oxm-match.h"
#include "openflow/openflow.h"

#include "ofl.h"
#include "ofl-actions.h"
#include "ofl-structs.h"
#include "ofl-print.h"
#include "ofl-packets.h"


#define ETH_ADDR_FMT                                                    \
    "%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8":%02"PRIx8
#define ETH_ADDR_ARGS(ea)                                   \
    (ea)[0], (ea)[1], (ea)[2], (ea)[3], (ea)[4], (ea)[5]

#define IP_FMT "%"PRIu8".%"PRIu8".%"PRIu8".%"PRIu8

char *
ofl_structs_port_to_string(struct ofl_port *port) {
        char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);
    ofl_structs_port_print(stream, port);
    fclose(stream);
    return str;
}

void
ofl_structs_port_print(FILE *stream, struct ofl_port *port) {
    fprintf(stream, "{no=\"");
    ofl_port_print(stream, port->port_no);
    fprintf(stream, "\", hw_addr=\""ETH_ADDR_FMT"\", name=\"%s\", "
                          "config=\"0x%"PRIx32"\", state=\"0x%"PRIx32"\", curr=\"0x%"PRIx32"\", "
                          "adv=\"0x%"PRIx32"\", supp=\"0x%"PRIx32"\", peer=\"0x%"PRIx32"\", "
                          "curr_spd=\"%ukbps\", max_spd=\"%ukbps\"}",
                  ETH_ADDR_ARGS(port->hw_addr), port->name,
                  port->config, port->state, port->curr,
                  port->advertised, port->supported, port->peer,
                  port->curr_speed, port->max_speed);
}

char *
ofl_structs_instruction_to_string(struct ofl_instruction_header *inst, struct ofl_exp *exp) {
        char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);
    ofl_structs_instruction_print(stream, inst, exp);
    fclose(stream);
    return str;
}

void
ofl_structs_instruction_print(FILE *stream, struct ofl_instruction_header *inst, struct ofl_exp *exp) {
    ofl_instruction_type_print(stream, inst->type);

    switch(inst->type) {
        case (OFPIT_GOTO_TABLE): {
            struct ofl_instruction_goto_table *i = (struct ofl_instruction_goto_table *)inst;

            fprintf(stream, "{table=\"%u\"}", i->table_id);

            break;
        }
        case (OFPIT_WRITE_METADATA): {
            struct ofl_instruction_write_metadata *i = (struct ofl_instruction_write_metadata *)inst;

            fprintf(stream, "{meta=\"0x%"PRIx64"\", mask=\"0x%"PRIx64"\"}",
                          i->metadata, i->metadata_mask);

            break;
        }
        case (OFPIT_WRITE_ACTIONS):
        case (OFPIT_APPLY_ACTIONS): {
            struct ofl_instruction_actions *i = (struct ofl_instruction_actions *)inst;
            size_t j;

            fprintf(stream, "{acts=[");
            for(j=0; j<i->actions_num; j++) {
                ofl_action_print(stream, i->actions[j], exp);
                if (j < i->actions_num - 1) { fprintf(stream, ", "); }
            }
            fprintf(stream, "]}");

            break;
        }
        case (OFPIT_CLEAR_ACTIONS): {
            break;
        }
        case (OFPIT_EXPERIMENTER): {
            if (exp == NULL || exp->inst == NULL || exp->inst->to_string == NULL) {
                struct ofl_instruction_experimenter *i = (struct ofl_instruction_experimenter *)inst;

                fprintf(stream, "{id=\"0x%"PRIx32"\"}", i->experimenter_id);
            } else {
                char *c = exp->inst->to_string(inst);
                fprintf(stream, "%s", c);
                free (c);
            }
            break;
        }
    }

}

char *
ofl_structs_match_to_string(struct ofl_match_header *match, struct ofl_exp *exp) {
    char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);
    ofl_structs_match_print(stream, match, exp);
    fclose(stream);
    return str;
}

void 
print_oxm_tlv(FILE *stream, struct ofl_match_tlv *f, size_t *size){
                
                if (f->header == OXM_OF_IN_PORT){
                    fprintf(stream, "in_port=%d",*((uint32_t*) f->value));
                    *size -= 8;   
                    if (*size > 4)                                  
                        fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_IN_PHY_PORT){
                            fprintf(stream, "in_phy_port=%d",*((uint32_t*) f->value));
                            *size -= 8;   
                            if (*size > 4)                                  
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_VLAN_VID){
                            if ((uint16_t) *f->value == OFPVID_NONE)
                                fprintf(stream, "vlan_vid= none");
                            else if ((uint16_t) *f->value == OFPVID_PRESENT)
                                fprintf(stream, "vlan_vid= present");
                            else fprintf(stream, "vlan_vid= %d",*((uint16_t*) f->value));
                            *size -= 6;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_VLAN_PCP){
                            fprintf(stream, "vlan_pcp= %d", *f->value & 0x7);
                            *size -= 5;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                } 
                else if (f->header == OXM_OF_ETH_TYPE){
                            uint16_t *v = (uint16_t *) f->value;
                            fprintf(stream, "eth_type=0x");
                            fprintf(stream,"%x",  *v);
                            *size -= 6; 
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_TCP_SRC){
                            fprintf(stream, "tcp_src=%d",*((uint16_t*) f->value));
                            *size -= 6;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_TCP_DST){
                            fprintf(stream, "tcp_dst=%d",*((uint16_t*) f->value));
                            *size -= 6;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_UDP_SRC){
                            fprintf(stream, "udp_src=%d",*((uint16_t*) f->value));
                            *size -= 6;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_UDP_DST){
                            fprintf(stream, "udp_dst=%d",*((uint16_t*) f->value));
                            *size -= 6;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_SCTP_SRC){
                            fprintf(stream, "sctp_src=%d",*((uint16_t*) f->value));
                            *size -= 6;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_SCTP_DST){
                            fprintf(stream, "sctp_dst=%d",*((uint16_t*) f->value));
                            *size -= 6;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }                  
                else if (f->header == OXM_OF_ETH_SRC || f->header == OXM_OF_ETH_SRC_W){
                            fprintf(stream, "eth_src=\""ETH_ADDR_FMT"\"", ETH_ADDR_ARGS(f->value));
                            *size -= 10;                                
                            if (OXM_HASMASK(f->header)){
                                *size -= 6;
                                fprintf(stream, "eth_src_mask=\""ETH_ADDR_FMT"\"", ETH_ADDR_ARGS(f->value + 6));
                            }
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_ETH_DST || f->header == OXM_OF_ETH_DST_W){
                            fprintf(stream, "eth_dst=\""ETH_ADDR_FMT"\"", ETH_ADDR_ARGS(f->value));
                            *size -= 10;                                
                            if (OXM_HASMASK(f->header)){
                                *size -= 6;
                                fprintf(stream, "eth_dst_mask=\""ETH_ADDR_FMT"\"", ETH_ADDR_ARGS(f->value + 6));
                            }
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }                                 
                else if (f->header == OXM_OF_IPV4_SRC || f->header == OXM_OF_IPV4_SRC_W){
                            fprintf(stream, "ipv4_src=\""IP_FMT"\"",IP_ARGS(f->value));
                            *size -= 8;                                
                            if (OXM_HASMASK(f->header)){
                                *size -= 4;
                                fprintf(stream, "ipv4_src_mask=\""IP_FMT"\"",IP_ARGS(f->value + 4));
                            }
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_IP_PROTO){
                            fprintf(stream, "ip_proto= %d", *f->value);
                            *size -= 5;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                } 
                else if (f->header == OXM_OF_IP_DSCP){
                            fprintf(stream, "ip_dscp= %d", *f->value & 0x3f);
                            *size -= 5;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                } 
                else if (f->header == OXM_OF_IP_ECN){
                            fprintf(stream, "ip_ecn= %d", *f->value & 0x3);
                            *size -= 5;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                } 
                else if (f->header == OXM_OF_ICMPV4_TYPE){
                            fprintf(stream, "icmpv4_type= %d", *f->value);
                            *size -= 5;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                } 
                else if (f->header == OXM_OF_ICMPV4_CODE){
                            fprintf(stream, "icmpv4_code= %d", *f->value);
                            *size -= 5;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }   
                else if (f->header == OXM_OF_IPV4_DST || f->header == OXM_OF_IPV4_DST_W){
                            fprintf(stream, "ipv4_dst=\""IP_FMT"\"",IP_ARGS(f->value));
                            *size -= 8;
                            if (OXM_HASMASK(f->header)){
                                *size -= 4;
                                fprintf(stream, "ipv4_dst_mask=\""IP_FMT"\"",IP_ARGS(f->value + 4));
                            }                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }   
                else if (f->header == OXM_OF_ARP_SPA || f->header == OXM_OF_ARP_SPA_W ){
                            fprintf(stream, "arp_sha=\""IP_FMT"\"",IP_ARGS(f->value));
                            *size -= 8;                                
                            if (OXM_HASMASK(f->header)){
                                *size -= 4;
                                fprintf(stream, "arp_sha_mask=\""IP_FMT"\"",IP_ARGS(f->value + 4));
                            }           
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                } 
                else if (f->header == OXM_OF_ARP_TPA || f->header == OXM_OF_ARP_TPA_W){
                            fprintf(stream, "arp_tpa=\""IP_FMT"\"",IP_ARGS(f->value));
                            *size -= 8;                                
                            if (OXM_HASMASK(f->header)){
                                *size -= 4;
                                fprintf(stream, "arp_tpa_mask=\""IP_FMT"\"",IP_ARGS(f->value + 4));
                            }
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                } 
                else if (f->header == OXM_OF_ARP_OP){
                            uint16_t *v = (uint16_t *) f->value;
                            fprintf(stream, "arp_op=0x");
                            fprintf(stream,"%x",  *v);
                            *size -= 6;
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_IPV6_SRC || f->header == OXM_OF_IPV6_SRC_W ){
                        char addr_str[INET6_ADDRSTRLEN]; 
                        inet_ntop(AF_INET6, f->value, addr_str, INET6_ADDRSTRLEN);
                        *size -= 20;
                        fprintf(stream, "nw_src_ipv6=\"%s\"", addr_str);
                        if (OXM_HASMASK(f->header)){
                                *size -= 16;
                                inet_ntop(AF_INET6, f->value + 16, addr_str, INET6_ADDRSTRLEN);
                                fprintf(stream, "nw_src_ipv6_mask=\"%s\"", addr_str);
                        }
                        if (*size > 4)                                
                                fprintf(stream, ", ");        
                }
                else if (f->header == OXM_OF_IPV6_DST || f->header == OXM_OF_IPV6_DST_W){
                        char addr_str[INET6_ADDRSTRLEN]; 
                        inet_ntop(AF_INET6, f->value, addr_str, INET6_ADDRSTRLEN);
                        *size -= 20;
                        fprintf(stream, "nw_dst_ipv6=\"%s\"", addr_str);
                        if (OXM_HASMASK(f->header)){
                                *size -= 16;
                                inet_ntop(AF_INET6, f->value + 16, addr_str, INET6_ADDRSTRLEN);
                                fprintf(stream, "nw_dst_ipv6_mask=\"%s\"", addr_str);
                        }
                        if (*size > 4)                                
                                fprintf(stream, ", ");        
                }
                else if (f->header == OXM_OF_IPV6_ND_TARGET){
                        char addr_str[INET6_ADDRSTRLEN]; 
                        inet_ntop(AF_INET6, f->value, addr_str, INET6_ADDRSTRLEN);
                        *size -= 20;
                        fprintf(stream, "ipv6_nd_target=\"%s\"", addr_str);
                        if (*size > 4)                                
                                fprintf(stream, ", ");        
                }  
                 else if (f->header == OXM_OF_IPV6_ND_SLL){
                            fprintf(stream, "ipv6_nd_sll=\""ETH_ADDR_FMT"\"", ETH_ADDR_ARGS(f->value));
                            *size -= 10;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_IPV6_ND_TLL){
                            fprintf(stream, "ipv6_nd_tll=\""ETH_ADDR_FMT"\"", ETH_ADDR_ARGS(f->value));
                            *size -= 10;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_IPV6_FLABEL){
                            uint32_t mask = 0xfffff;
                            fprintf(stream, "ipv6_flow_label=%x",((uint32_t) *f->value) & mask );
                            *size -= 8;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_ICMPV6_TYPE){
                            fprintf(stream, "icmpv6_type= %d", *f->value);
                            *size -= 5;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                } 
                else if (f->header == OXM_OF_ICMPV6_CODE){
                            fprintf(stream, "icmpv6_code= %d", *f->value);
                            *size -= 5;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_MPLS_LABEL){
                            uint32_t mask = 0xfffff;
                            fprintf(stream, "mpls_label=%x",((uint32_t) *f->value) & mask );
                            *size -= 8;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }
                else if (f->header == OXM_OF_MPLS_TC){
                            fprintf(stream, "mpls_tc= %d", *f->value & 0x3);
                            *size -= 5;                                
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                } 
                else if (f->header == OXM_OF_METADATA || f->header == OXM_OF_METADATA_W){
                            fprintf(stream, "metadata= %lld", (uint64_t) *f->value);
                            *size -= 12;
                            if (OXM_HASMASK(f->header)){
                                fprintf(stream, "metadata_mask= %lld", (uint64_t) *(f->value + 8));
                                *size -= 8;
                            }                            
                            if (*size > 4)                                
                                fprintf(stream, ", ");
                }                                                                 
}

static void print_oxm_match(FILE *stream, struct ofl_match *m){
        struct ofl_match_tlv   *f;
        size_t size = m->header.length;
        fprintf(stream, "oxm{");
        if (size) {
            /*TODO: Create a mapping of header values and names to avoid so many comparisons */ 
            HMAP_FOR_EACH(f, struct ofl_match_tlv, hmap_node, &m->match_fields){                             
                print_oxm_tlv(stream, f, &size);
            }
        }    
        else fprintf(stream, "all match");
        fprintf(stream, "}");
}

void
ofl_structs_match_print(FILE *stream, struct ofl_match_header *match, struct ofl_exp *exp) {

    switch (match->type) {
        case (OFPMT_OXM): {
            struct ofl_match *m = (struct ofl_match*) match;
            print_oxm_match(stream, m);
            break;
        }
        default: {
            if (exp == NULL || exp->match == NULL || exp->match->to_string == NULL) {
                fprintf(stream, "?(%u)", match->type);
            } else {
                char *c = exp->match->to_string(match);
                fprintf(stream, "%s", c);
                free(c);
            }
        }
    }
}


char *
ofl_structs_config_to_string(struct ofl_config *c) {
        char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);
    ofl_structs_config_print(stream, c);
    fclose(stream);
    return str;
}

void
ofl_structs_config_print(FILE *stream, struct ofl_config *c) {
    fprintf(stream, "{flags=\"0x%"PRIx16"\", mlen=\"%u\"}",
                  c->flags, c->miss_send_len);
}

char *
ofl_structs_bucket_to_string(struct ofl_bucket *b, struct ofl_exp *exp) {
        char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);
    ofl_structs_bucket_print(stream, b, exp);
    fclose(stream);
    return str;
}

void
ofl_structs_bucket_print(FILE *stream, struct ofl_bucket *b, struct ofl_exp *exp) {
    size_t i;

    fprintf(stream, "{w=\"%u\", wprt=\"", b->weight);
    ofl_port_print(stream, b->watch_port);
    fprintf(stream, "\", wgrp=\"");
    ofl_group_print(stream, b->watch_group);
    fprintf(stream, "\", acts=[");

    for (i=0; i<b->actions_num; i++) {
        ofl_action_print(stream, b->actions[i], exp);
        if (i < b->actions_num - 1) { fprintf(stream, ", "); }
    }

    fprintf(stream, "]}");
}

char *
ofl_structs_queue_to_string(struct ofl_packet_queue *q) {
        char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);
    ofl_structs_queue_print(stream, q);
    fclose(stream);
    return str;
}

void
ofl_structs_queue_print(FILE *stream, struct ofl_packet_queue *q) {
    size_t i;

    fprintf(stream, "{q=\"");
    ofl_queue_print(stream, q->queue_id);
    fprintf(stream, "\", props=[");

    for (i=0; i<q->properties_num; i++) {
        ofl_structs_queue_prop_print(stream, q->properties[i]);
        if (i < q->properties_num - 1) { fprintf(stream, ", "); }
    }

    fprintf(stream, "]}");
}

char *
ofl_structs_queue_prop_to_string(struct ofl_queue_prop_header *p) {
        char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);
    ofl_structs_queue_prop_print(stream, p);
    fclose(stream);
    return str;
}

void
ofl_structs_queue_prop_print(FILE *stream, struct ofl_queue_prop_header *p) {
    ofl_queue_prop_type_print(stream, p->type);

    switch(p->type) {
        case (OFPQT_MIN_RATE): {
            struct ofl_queue_prop_min_rate *pm = (struct ofl_queue_prop_min_rate *)p;

            fprintf(stream, "{rate=\"%u\"}", pm->rate);
            break;
        }
        
    }

}

char *
ofl_structs_flow_stats_to_string(struct ofl_flow_stats *s, struct ofl_exp *exp) {
        char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);
    ofl_structs_flow_stats_print(stream, s, exp);
    fclose(stream);
    return str;
}

void
ofl_structs_flow_stats_print(FILE *stream, struct ofl_flow_stats *s, struct ofl_exp *exp) {
    size_t i;

    fprintf(stream, "{table=\"");
    ofl_table_print(stream, s->table_id);
    fprintf(stream, "\", match=\"");
    ofl_structs_match_print(stream, s->match, exp);
    fprintf(stream, "\", dur_s=\"%u\", dur_ns=\"%u\", prio=\"%u\", "
                          "idle_to=\"%u\", hard_to=\"%u\", cookie=\"0x%"PRIx64"\", "
                          "pkt_cnt=\"%"PRIu64"\", byte_cnt=\"%"PRIu64"\", insts=[",
                  s->duration_sec, s->duration_nsec, s->priority,
                  s->idle_timeout, s->hard_timeout, s->cookie,
                  s->packet_count, s->byte_count);

    for (i=0; i<s->instructions_num; i++) {
        ofl_structs_instruction_print(stream, s->instructions[i], exp);
        if (i < s->instructions_num - 1) { fprintf(stream, ", "); };
    }

    fprintf(stream, "]}");
}

char *
ofl_structs_bucket_counter_to_string(struct ofl_bucket_counter *s) {
        char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);
    ofl_structs_bucket_counter_print(stream, s);
    fclose(stream);
    return str;
}

void
ofl_structs_bucket_counter_print(FILE *stream, struct ofl_bucket_counter *c) {
    fprintf(stream, "{pkt_cnt=\"%"PRIu64"\", byte_cnt=\"%"PRIu64"\"}",
                  c->packet_count, c->byte_count);
}

char *
ofl_structs_group_stats_to_string(struct ofl_group_stats *s) {
        char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);
    ofl_structs_group_stats_print(stream, s);
    fclose(stream);
    return str;
}

void
ofl_structs_group_stats_print(FILE *stream, struct ofl_group_stats *s) {
    size_t i;

    fprintf(stream, "{group=\"");
    ofl_group_print(stream, s->group_id);
    fprintf(stream, "\", ref_cnt=\"%u\", pkt_cnt=\"%"PRIu64"\", byte_cnt=\"%"PRIu64"\", cntrs=[",
                  s->ref_count, s->packet_count, s->byte_count);

    for (i=0; i<s->counters_num; i++) {
        ofl_structs_bucket_counter_print(stream, s->counters[i]);
        if (i < s->counters_num - 1) { fprintf(stream, ", "); };
    }

    fprintf(stream, "]}");
}

char *
ofl_structs_table_stats_to_string(struct ofl_table_stats *s) {
    char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);

    ofl_structs_table_stats_print(stream, s);

    fclose(stream);
    return str;
}

void
ofl_structs_table_stats_print(FILE *stream, struct ofl_table_stats *s) {
    fprintf(stream, "{table=\"");
    ofl_table_print(stream, s->table_id);
    fprintf(stream, "\", name=\"%s\", wcards=\"0x%"PRIx32"\", match=\"0x%"PRIx32"\", "
                          "insts=\"0x%"PRIx32"\", w_acts=\"0x%"PRIx32"\", a_acts=\"0x%"PRIx32"\", "
                          "a_set=\"0x%"PRIx64"\", w_set=\"0x%"PRIx64"\", m_match=\"0x%"PRIx64"\", "
                          "m_write=\"0x%"PRIx64"\", conf=\"0x%"PRIx32"\", max=\"%u\", active=\"%u\", "
                          "lookup=\"%"PRIu64"\", match=\"%"PRIu64"\"",
                  s->name, s->wildcards, s->match,
                  s->instructions, s->write_actions, s->apply_actions,
                  s->write_setfields, s->apply_setfields, s->metadata_match,
                  s->metadata_write,s->config, s->max_entries, s->active_count,
                  s->lookup_count, s->matched_count);
}

char *
ofl_structs_port_stats_to_string(struct ofl_port_stats *s) {
        char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);
    ofl_structs_port_stats_print(stream, s);
    fclose(stream);
    return str;
}

void
ofl_structs_port_stats_print(FILE *stream, struct ofl_port_stats *s) {

    fprintf(stream, "{port=\"");
    ofl_port_print(stream, s->port_no);
    fprintf(stream, "\", rx_pkt=\"%"PRIu64"\", tx_pkt=\"%"PRIu64"\", "
                          "rx_bytes=\"%"PRIu64"\", tx_bytes=\"%"PRIu64"\", "
                          "rx_drops=\"%"PRIu64"\", tx_drops=\"%"PRIu64"\", "
                          "rx_errs=\"%"PRIu64"\", tx_errs=\"%"PRIu64"\", "
                          "rx_frm=\"%"PRIu64"\", rx_over=\"%"PRIu64"\", "
                          "rx_crc=\"%"PRIu64"\", coll=\"%"PRIu64"\"}",
                  s->rx_packets, s->tx_packets,
                  s->rx_bytes, s->tx_bytes,
                  s->rx_dropped, s->tx_dropped,
                  s->rx_errors, s->tx_errors,
                  s->rx_frame_err, s->rx_over_err,
                  s->rx_crc_err, s->collisions);
};

char *
ofl_structs_queue_stats_to_string(struct ofl_queue_stats *s) {
        char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);
    ofl_structs_queue_stats_print(stream, s);
    fclose(stream);
    return str;
}

void
ofl_structs_queue_stats_print(FILE *stream, struct ofl_queue_stats *s) {

    fprintf(stream, "{port=\"");
    ofl_port_print(stream, s->port_no);
    fprintf(stream, "\", q=\"");
    ofl_queue_print(stream, s->queue_id);
    fprintf(stream, "\", tx_bytes=\"%"PRIu64"\", "
                          "tx_pkt=\"%"PRIu64"\", tx_err=\"%"PRIu64"\"}",
                  s->tx_bytes, s->tx_packets, s->tx_errors);
};

char *
ofl_structs_group_desc_stats_to_string(struct ofl_group_desc_stats *s, struct ofl_exp *exp) {
        char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);
    ofl_structs_group_desc_stats_print(stream, s, exp);
    fclose(stream);
    return str;
}

void
ofl_structs_group_desc_stats_print(FILE *stream, struct ofl_group_desc_stats *s, struct ofl_exp *exp) {
    size_t i;

    fprintf(stream, "{type=\"");
    ofl_group_type_print(stream, s->type);
    fprintf(stream, "\", group=\"");
    ofl_group_print(stream, s->group_id);
    fprintf(stream, "\", buckets=[");

    for (i=0; i<s->buckets_num; i++) {
        ofl_structs_bucket_print(stream, s->buckets[i], exp);
        if (i < s->buckets_num - 1) { fprintf(stream, ", "); };
    }

    fprintf(stream, "]}");
}
