
#pragma once

#include "../external_include.h"
//
#include "polygon.h"

struct NavMesh {
    std::map<int, Polygon> entityShapes;
    std::vector<Polygon> shapes;

    void addEntity(int e_id, Polygon p) { entityShapes.insert({e_id, p}); }

    void removeEntity(int e_id) {
        auto it = entityShapes.find(e_id);
        if (it != entityShapes.end()) entityShapes.erase(it);
    }

    void addShape(const Polygon& p, bool merge = true) {
        shapes.push_back(p);

        if (!merge) return;

        size_t i = 0;
        size_t j = 1;
        while (i < shapes.size()) {
            j = i + 1;
            while (j < shapes.size()) {
                if (overlap(shapes[i], shapes[j])) {
                    for (auto pt : shapes[j].points) {
                        shapes[i].add(pt);
                    }
                    shapes.erase(shapes.begin() + j);
                } else {
                    j++;
                }
            }
            i++;
        }
    }

    void removeShape(const Polygon& p) {
        // TODO this might split the shape in two
        // how do we figure out if we need to split ...
        for (auto& s : shapes) {
            if (overlap(s, p)) {
                for (auto pt : p.points) {
                    s.remove(pt);
                }
            }
        }
    }

    bool overlap(const Polygon& a, const Polygon& b) const {
        // first check if the two max radii circles overlap
        for (size_t i = 0; i < a.points.size(); i++) {
            if (b.inside(a.points[i])) return true;
        }
        return false;
    }
};
