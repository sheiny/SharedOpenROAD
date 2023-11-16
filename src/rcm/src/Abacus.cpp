#include "rcm/Abacus.h"
#include "odb/db.h"
#include "ord/OpenRoad.hh"

#include <limits>
#include <algorithm>
#include <vector>
#include <iostream>


namespace rcm {
    using namespace odb;
    using std::vector, std::sort, std::lower_bound, std::upper_bound, std::make_pair, std::numeric_limits;

    Abacus::Abacus() :
        db{ord::OpenRoad::openRoad()->getDb()}
        {}

    void Abacus::abacus(int x1, int y1, int x2, int y2) {
        std::cout<<"Initializing Abacus ..."<<std::endl;
        int area_x_min, area_x_max, area_y_min, area_y_max;
        if (x1 < x2) {
            area_x_min = x1;
            area_x_max = x2;
        } else {
            area_x_max = x1;
            area_x_min = x2;
        }
        if (y1 < y2) {
            area_y_min = y1;
            area_y_max = y2;
        } else {
            area_y_max = y1;
            area_y_min = y2;
        }

        dbBlock* block = db->getChip()->getBlock();

        vector<Cell> cells;
        vector<Rect> fixed_cells;
        {
            dbSet<dbInst> insts = block->getInsts();
            
            for (dbInst* inst : insts) {
                Rect rect = inst->getBBox()->getBox();

                if (!collide(rect.xMin(), rect.xMax(), area_x_min, area_x_max)
                    || !collide(rect.yMin(), rect.yMax(), area_y_min, area_y_max)) {
                    continue;
                }

                if (area_x_min <= rect.xMin() && rect.xMax() <= area_x_max
                    && area_y_min <= rect.yMin() && rect.yMax() <= area_y_max
                ) {
                    cells.push_back(make_pair(rect, inst));
                } else {
                    fixed_cells.push_back(rect);
                }
            }
        }

        vector<Row> rows;
        vector<vector<Split>> splits_per_row;
        {
            dbSet<dbRow> rows_set = block->getRows();
            for (dbRow* row : rows_set) {
                Rect rect = row->getBBox();
                if (collide(area_x_min, area_x_max, rect.xMin(), rect.xMax())
                    && collide(area_y_min, area_y_max, rect.yMin(), rect.yMax())
                ) {
                    int site_width = row->getSite()->getWidth();

                    int site_x_min = (area_x_min - rect.xMin()) / site_width * site_width + rect.xMin();
                    int site_x_max = (area_x_max - rect.xMin()) / site_width * site_width + rect.xMin();

                    rect.set_xlo(std::max(site_x_min, rect.xMin()));
                    rect.set_xhi(std::min(site_x_max, rect.xMax()));
                    rows.push_back({rect, site_width});
                }
            }

            splits_per_row = sort_and_get_splits(&rows, fixed_cells);
        }



        abacus(rows, splits_per_row, &cells);
    }

    void Abacus::abacus(
        vector<Row> const& rows,
        vector<vector<Split>> const& splits_per_row,
        vector<Cell>* cells
    ) {
        std::cout<<"Running abacus..."<<std::endl;
        sort(cells->begin(), cells->end(),
            [&](Cell const& a, Cell const& b) {
                return a.first.xMin() < b.first.xMin();
            }
        );

        vector<int> row_to_start_split(rows.size());
        int total_splits = 0;
        for (int row_i = 0; row_i < rows.size(); row_i++) {
            row_to_start_split[row_i] = total_splits;
            total_splits += splits_per_row[row_i].size();
        }

        vector<vector<int>> cells_per_accum_split(total_splits);
        vector<vector<AbacusCluster>> clusters_per_accum_split(total_splits);

        for (int cell_i = 0; cell_i < cells->size(); cell_i++) {
            auto const& [global_pos, inst] = (*cells)[cell_i];

            double best_cost = std::numeric_limits<double>::max();
            int best_row_i = -1;
            int best_split_i = -1;
            AbacusCluster best_new_cluster;
            int best_previous_i = 0;

            auto evaluate = [&](int row_i, double y_cost, int split_i) {
                auto const& [whole_row, site_width] = rows[row_i];
                auto [x_min, x_max] = splits_per_row[row_i][split_i];
                Rect row(x_min, whole_row.yMin(), x_max, whole_row.yMax());

                double weight = global_pos.dx()*global_pos.dy();
                AbacusCell cell = {cell_i, global_pos, weight};

                AbacusCluster new_cluster;
                int previous_i;

                int accum_split_i = row_to_start_split[row_i] + split_i;
                if (!abacus_try_add_cell(
                    row, site_width,
                    cell,
                    clusters_per_accum_split[accum_split_i],
                    &new_cluster, &previous_i
                )) {
                    return;
                }

                int new_x = new_cluster.x + new_cluster.width - global_pos.dx();
                double sqrt_x_cost = new_x - global_pos.xMin();
                double x_cost = sqrt_x_cost*sqrt_x_cost;

                double curr_cost = x_cost + y_cost;

                if (curr_cost < best_cost) {
                    best_cost = curr_cost;
                    best_row_i = row_i;
                    best_split_i = split_i;
                    best_new_cluster = new_cluster;
                    best_previous_i = previous_i;
                }
            };

            auto loop_y = [&](int row_i) -> bool {
                Rect const& row_rect = rows[row_i].first;
                double sqrt_y_cost = row_rect.yMin() - global_pos.yMin();
                double y_cost = sqrt_y_cost*sqrt_y_cost;
                if (y_cost > best_cost) return false;

                vector<Split> const& splits = splits_per_row[row_i];

                auto loop_x = [&](int split_i, bool new_row, int comp_x) {
                    if (!new_row) {
                        double sqrt_x_cost = comp_x - global_pos.xMin();
                        double x_cost = sqrt_x_cost*sqrt_x_cost;
                        if (x_cost > best_cost) return false;
                    }

                    evaluate(row_i, y_cost, split_i);

                    return true;
                };

                int approx_split = std::lower_bound(splits.begin(), splits.end(),
                    make_pair(0, global_pos.xMin()),
                    [&](Split const& a, Split const& b) {
                        return a.second < b.second;
                    }
                ) - splits.begin();

                bool new_row = true;
                for (int split_i = approx_split; split_i < splits.size(); split_i++) {
                    int x_min = splits[split_i].first;
                    if (!loop_x(split_i, new_row, x_min)) break;
                    new_row = false;
                }
                for (int split_i = approx_split-1; split_i >= 0; split_i--) {
                    int x_max = splits[split_i].second;
                    if (!loop_x(split_i, new_row, x_max - global_pos.dx())) break;
                    new_row = false;
                }

                return true;
            };

            Rect dummy_rect = Rect(
                0, global_pos.yMin()-1,
                1, global_pos.yMin()
            );
            int approx_row = std::upper_bound(
                rows.begin(), rows.end(), make_pair(dummy_rect, 0),
                [&](Row const& a, Row const& b) {
                    return a.first.yMax() < b.first.yMax();
                }
            ) - rows.begin();

            for (int row_i = approx_row; row_i < rows.size(); row_i++) {
                if (!loop_y(row_i)) break;;
            }
            for (int row_i = approx_row-1; row_i >= 0; row_i--) {
                if (!loop_y(row_i)) break;
            }

            if (best_row_i == -1) {
                fprintf(stderr, "ERROR: could not place cell\n");
            } else {
                int accum_split_i =
                    row_to_start_split[best_row_i] + best_split_i;
                cells_per_accum_split[accum_split_i]
                    .push_back(cell_i);
                clusters_per_accum_split[accum_split_i]
                    .resize(best_previous_i+1);
                clusters_per_accum_split[accum_split_i]
                    .push_back(best_new_cluster);
            }
        }

        int accum_split_i = 0;
        for (int row_i = 0; row_i < rows.size(); row_i++) {
            vector<Split> const& splits = splits_per_row[row_i];
            for (int split_i = 0; split_i < splits.size(); split_i++) {
                vector<int> const& cells_in_row
                    = cells_per_accum_split[accum_split_i];
                vector<AbacusCluster> const& clusters
                    = clusters_per_accum_split[accum_split_i];

                int split_cell_i = 0;
                for (AbacusCluster const& cluster : clusters) {
                    int x = cluster.x;
                    while (split_cell_i <= cluster.last_cell) {
                        int cell_i = cells_in_row[split_cell_i];
                        auto const& [cell, inst] = (*cells)[cell_i];

                        Rect const& row = rows[row_i].first;

                        set_pos(inst, x, row.yMin());

                        x += cell.dx();
                        split_cell_i++;
                    }
                }

                accum_split_i += 1;
            }
        }
    }

    bool Abacus::abacus_try_add_cell(
        Rect row, int site_width,
        AbacusCell const& cell,
        vector<AbacusCluster> const& clusters,
        AbacusCluster* new_cluster, int* previous_i
    ) {
        if (clusters.size() == 0
            || clusters.back().x + clusters.back().width < cell.global_pos.xMin()
        ) {
            int last_cell_dec = -1;
            if (clusters.size() > 0) last_cell_dec = clusters.back().last_cell;

            *new_cluster = {0, 0, 0, cell.global_pos.xMin(), last_cell_dec};
            *previous_i = clusters.size() - 1;
        } else {
            *new_cluster = clusters.back();
            *previous_i = clusters.size() - 2;
        }

        {
            new_cluster->q      += cell.weight * (cell.global_pos.xMin() - new_cluster->width);
            new_cluster->weight += cell.weight;
            new_cluster->width  += cell.global_pos.dx();
            new_cluster->last_cell++;
        }

        if (!abacus_try_place_and_collapse(
            clusters,
            row, site_width,
            new_cluster, previous_i)
        ) {
            return false;
        }

        return true;
    }

    bool Abacus::abacus_try_place_and_collapse(
        vector<AbacusCluster> const& clusters,
        Rect row, int site_width,
        AbacusCluster* new_cluster, int* previous_i
    ) {
        if (new_cluster->width > row.dx()) return false;

        new_cluster->x = new_cluster->q / new_cluster->weight;
        new_cluster->x = (new_cluster->x - row.xMin()) / site_width * site_width + row.xMin();

        new_cluster->x = std::clamp(
            new_cluster->x,
            row.xMin(),
            (row.xMax() - new_cluster->width)
        );

        if (*previous_i >= 0) {
            AbacusCluster const& previous = clusters[*previous_i];
            if (previous.x + previous.width > new_cluster->x) {
                {
                    new_cluster->q = previous.q + new_cluster->q - new_cluster->weight * previous.width;
                    new_cluster->weight = previous.weight + new_cluster->weight;
                    new_cluster->width  = previous.width  + new_cluster->width;

                    (*previous_i)--;
                }
                if (!abacus_try_place_and_collapse(
                    clusters, row, site_width,
                    new_cluster, previous_i)
                ) {
                    return false;
                }
            }
        }

        return true;
    }

    void Abacus::set_pos(dbInst* cell, int x, int y) {
        cell->setLocation(x, y);
    };

    bool Abacus::collide(int pos1_min, int pos1_max, int pos2_min, int pos2_max) {
        return pos1_min < pos2_max && pos2_min < pos1_max;
    };

    auto Abacus::sort_and_get_splits(
        vector<Row>* rows,
        vector<Rect> const& fixed_cells
    ) -> vector<vector<Split>> {
        sort(rows->begin(), rows->end(),
            [&](Row const& a, Row const& b) {
                return a.first.yMin() < b.first.yMin();
            }
        );

        vector<vector<Split>> splits_per_row(rows->size());
        for (int row_i = 0; row_i < rows->size(); row_i++) {
            Rect const& rect = (*rows)[row_i].first;
            splits_per_row[row_i].emplace_back(rect.xMin(), rect.xMax());
        }

        for (Rect const& fixed_cell : fixed_cells) {
            int int_min = numeric_limits<int>::min();
            int int_max = numeric_limits<int>::max();

            Rect dummy_row1 = Rect(
                int_min, int_min,
                int_max, fixed_cell.yMin()
            );
            int row_start = std::upper_bound(rows->begin(), rows->end(),
                make_pair(dummy_row1, 0),
                [&](Row const& a, Row const& b) {
                    return a.first.yMax() < b.first.yMax();
                }
            ) - rows->begin();

            Rect dummy_row2 = Rect(
                int_min, int_min,
                int_max, fixed_cell.yMax()
            );
            int row_end = std::lower_bound(rows->begin(), rows->end(),
                make_pair(dummy_row2, 0),
                [&](Row const& a, Row const& b) {
                    return a.first.yMax() < b.first.yMax();
                }
            ) - rows->begin();

            if (row_end == rows->size()) row_end--;

            for (int row_i = row_start; row_i <= row_end; row_i++) {
                vector<Split>* splits = &splits_per_row[row_i];
                auto const& [row, site_width] = (*rows)[row_i];

                auto split = lower_bound(splits->begin(), splits->end(),
                    make_pair(0, fixed_cell.xMin()),
                    [&](Split const& a, Split const& b) {
                        return a.second < b.second;
                    }
                );
                if (split == splits->end()) continue;

                auto [old_x_min, old_x_max] = *split;

                if (!collide(fixed_cell.xMin(), fixed_cell.xMax(), old_x_min, old_x_max)) {
                    continue;
                }

                int new_x_min =
                    (fixed_cell.xMin() - row.xMin())
                    / site_width * site_width + row.xMin();
                int new_x_max = 
                    ((fixed_cell.xMax() - row.xMin()) + site_width-1)
                    / site_width * site_width + row.xMin();

                splits->erase(split);

                if (new_x_max < old_x_max) {
                    splits->insert(split, make_pair(new_x_max, old_x_max));
                }
                if (old_x_min < new_x_min) {
                    splits->insert(split, make_pair(old_x_min, new_x_min));
                }
            }
        }

        return splits_per_row;
    }
}