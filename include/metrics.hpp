#pragma once

#include <limits>

#include "mesh.hpp"

namespace Metrics
{

namespace Detail
{
    float INF = std::numeric_limits<float>::infinity();
}

class Metrics
{
    float hausdorff_;
    float rms_;
    float norm_hausdorff_;
    float norm_rms_;
    Mesh::Mesh orig_mesh_;
    Mesh::Mesh simple_mesh_;

public:

    Metrics(const Mesh::Mesh& orig_mesh, const Mesh::Mesh& simple_mesh) : orig_mesh_(orig_mesh), simple_mesh_(simple_mesh) 
    {
        count_hausdorff();
        count_rms();

        size_t diag = orig_mesh_.get_bounding_box_diag_size();

        norm_hausdorff_ = hausdorff_ / diag;
        norm_rms_ = rms_ / diag;
    }

    void print()
    {
        std::cout << "Hausdorf " << norm_hausdorff_ * 100 << "%\nRMS " << norm_rms_ * 100 <<  "%" << std::endl;
    }

    float get_hausdorff() { return hausdorff_; }
    float get_rms() { return rms_; }

private:

    float count_hausdorff()
    {
        float orig_simple_max_dist = 0.0f;
        auto& orig_vert = orig_mesh_.get_vertices();
        auto& simple_vert = simple_mesh_.get_vertices();

        for (const auto& s_v : simple_vert) 
        {
            float min_dist = Detail::INF;
            for (const auto& o_v : orig_vert) 
            {
                float dist = Vec3::distance(s_v, o_v);
                if (dist < min_dist)
                    min_dist = dist;
            }

            if (min_dist > orig_simple_max_dist)
                orig_simple_max_dist = min_dist;
        }

        hausdorff_ = orig_simple_max_dist;

        return hausdorff_;
    }

    float count_rms()
    {
        float orig_simple_sum_dist_sq = 0.0f;
        auto& orig_vert = orig_mesh_.get_vertices();
        auto& simple_vert = simple_mesh_.get_vertices();
        size_t s_v_amt = simple_mesh_.get_vert_amt();

        for (const auto& s_v : simple_vert) 
        {
            float min_dist = Detail::INF;
            for (const auto& o_v : orig_vert) 
            {
                float dist = Vec3::distance(s_v, o_v);
                if (dist < min_dist)
                    min_dist = dist;
            }
            orig_simple_sum_dist_sq += min_dist * min_dist;
        }

        rms_ = sqrt(orig_simple_sum_dist_sq / s_v_amt);

        return rms_;
    }
};

}