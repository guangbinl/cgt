//
// Created by adminstrator on 2022/9/19.
//
#pragma once

#include <cgt/cgtcore.h>
#include <osg/Node>

#include <gdal_frmts.h>
#include <ogrsf_frmts.h>
#include <vector>

namespace scially {
    class CGTLIBRARY osg_export : public osg_base {
    public:
        osg_export(const std::string &source_dir, const std::string &target_dir)
                : osg_base(source_dir, target_dir), outer_cut(true){

        }

        void set_extent(const std::string &shp);

        void set_copy(bool copy) { is_copy_ = copy; }

        virtual bool root_process(cgt_proj &proj, const std::string &tile_path) override;

        virtual bool tile_process(cgt_proj &proj, const std::string &tile_path) override;

        virtual bool end_process() override;

        virtual ~osg_export() {}

        void set_outer_flag(const bool& flag);

    private:
        bool is_intersect(const osg::Node &node);

        std::string shpfile_;
        std::unique_ptr<OGRGeometry> geometry_;
        std::unique_ptr<cgt_proj> proj_;
        std::vector<std::string> tiles;
        bool is_copy_ = true;
        bool outer_cut;
    };
}