#pragma once

#include "external_include.h"
#include "ui_color.h"
//
#include "entity.h"

struct Spawner : public Entity {
    Spawner(vec2 location) : Entity(location, ui::color::pine_green) {}
};

#include "customer.h"
#include "timer.h"
struct CustomerSpawner : public Spawner {
    CustomerSpawner(vec2 location) : Spawner(location) {}
    virtual bool is_collidable() override { return false; }
    virtual bool draw_outside_debug_mode() const override { return false; }

    void spawn() {
        std::shared_ptr<Customer> customer;
        customer.reset(new Customer(this->position, RED));
        EntityHelper::addEntity(customer);
    }
};
