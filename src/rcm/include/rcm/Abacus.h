#pragma once

#include "odb/db.h"

#include <vector>

namespace odb {
  class dbDatabase;
  class dbInst;
}

namespace rcm {

class Abacus {
    using Cell = std::pair<odb::Rect, odb::dbInst*>;
    using Row = std::pair<odb::Rect, int>;
    using Split = std::pair<int, int>;

public:
    Abacus();

    std::vector<odb::dbInst *> abacus(int x1, int y1, int x2, int y2);
    std::vector<odb::dbInst *> abacus(
        std::vector<Row> const& rows,
        std::vector<std::vector<Split>> const& splits_per_row,
        std::vector<Cell>* cells
    );

private:
    struct AbacusCluster {
        double q;
        double weight;
        int width;
        int x;
        int last_cell;
    };

    struct AbacusCell {
        int id;
        odb::Rect global_pos;
        double weight;
    };

    bool abacus_try_add_cell(
        odb::Rect row, int site_width,
        AbacusCell const& cell,
        std::vector<AbacusCluster> const& clusters,
        AbacusCluster* new_cluster, int* previous_i
    );
    bool abacus_try_place_and_collapse(
        std::vector<AbacusCluster> const& clusters,
        odb::Rect row, int site_width,
        AbacusCluster* new_cluster, int* previous_i
    );

    bool set_pos(odb::dbInst* cell, int x, int y);

    bool collide(int pos1_min, int pos1_max, int pos2_min, int pos2_max);

    auto sort_and_get_splits(
        std::vector<Row>* rows,
        std::vector<odb::Rect> const& fixed_cells
    ) -> std::vector<std::vector<Split>>;

    // attributes
    odb::dbDatabase* db;
};
}