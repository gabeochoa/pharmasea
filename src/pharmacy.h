
#pragma once 

#include "external_include.h"
#include "customer.h"
#include "singleton.h"


enum Location {
    OffScreen,
    Outside,
    WaitingInLine,
    Browsing,
    BeingHelped,
    Paying,
    Chatting,
    Leaving
};

SINGLETON_FWD(Pharmacy);
struct Pharmacy {
    SINGLETON(Pharmacy);

    float money;
    std::vector<std::shared_ptr<Customer>> customers;

    struct Day {
        float current_time;
        float num_customers;
        float day_length;
    } current_day;

};
