#ifndef OPENFLOW_LOADBALANCER_H
#define OPENFLOW_LOADBALANCER_H

#define OF_DEFAULT_SEARVER_NUMBER 4

enum oflb_type {
  /* The type of Load Balancer. */
  OFLB_RANDOM,
  OFLB_ROUND_ROBIN,
  OFLB_IP_HASHING
};

#endif
