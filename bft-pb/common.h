#pragma once
#include "msg_format.h"
#include "rpc.h"
#include <stdio.h>

static const std::string kdonna = "129.215.165.54";
static const std::string kmartha = "129.215.165.53";
static const std::string kamy = "129.215.165.57";

static constexpr uint16_t kUDPPort = 31850;
static constexpr size_t kMsgSize = sizeof(p_msg); // get_msg_buf_sz();
static constexpr size_t kAckSize =
    sizeof(ack_msg); // maybe put a get_ack_buf_sz();

static void sm_handler(int local_session, erpc::SmEventType, erpc::SmErrType,
                       void *) {}
