//
// Created by adminstrator on 2022/9/6.
//

#pragma once

#include <cgt/cgtmodel.h>
#include <spdlog/spdlog.h>
#include <osg/Node>
#include <osg/CoordinateSystemNode>
#include <osgDB/ConvertUTF>

#include <gdal_frmts.h>
#include <ogrsf_frmts.h>
#include <string>
#include <memory>
#include <functional>

namespace scially {
    class CGTLIBRARY gdal_init {
    public:
        gdal_init();

    private:
        std::string gdal_data_;
        std::string proj_data_;
    };

    class CGTLIBRARY cgt_proj {
    public:
        cgt_proj(const osg_modeldata &source_modeldata, osg_modeldata &target_modeldata, bool cal_dest_orign=false);

        osg::Vec3d transfrom(osg::Vec3d vert) const;

        void transform(OGRGeometry *geom) const;

    private:
        OGRSpatialReference import(const std::string &srs) const;

        void crs_to_proj(const std::string &source_crs,
                         std::string &proj_crs, osg::Vec3d &topcenteric, bool &is_topcenteric) const noexcept;
        osg::Vec3d pj_transorm(osg::Vec3d vert, bool forward = true) const;
        osg::Vec3d pj_transorm(double x, double y, double z, bool forward = true) const;

        osg_modeldata source_metadata_;
        osg_modeldata target_metadata_;

        std::function<osg::Vec3d(osg::Vec3d)> transformer_;
        OGRSpatialReference source_srs_;
        OGRSpatialReference target_srs_;
        osg::Matrixd source_local_to_world_;
        osg::Matrixd target_world_to_local_;
        osg::EllipsoidModel ellipsoid_;

        bool source_srs_is_topcenteric_ = false;
        bool target_srs_is_topcenteric_ = false;
        osg::Vec3d source_topcenteric_;
        osg::Vec3d target_topcenteric_;
        std::unique_ptr<OGRCoordinateTransformation> transform_srs_;
    };
}