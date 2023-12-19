//
// Created by adminstrator on 2022/9/19.
//

#include <cgt/cgtexport.h>
#include <osgDB/ReadFile>

#include <filesystem>

namespace scially {
    namespace fs = std::filesystem;

    struct gdal_dataset_deleter {
        void operator()(GDALDataset *ds) noexcept {
            GDALClose(ds);
        }
    }; // struct gdal_dataset_deleter

    struct ogr_feature_deleter {
        void operator()(OGRFeature* feature) noexcept {
            OGRFeature::DestroyFeature(feature);
        }
    }; // struct ogr_feature_deleter

    void osg_export::set_extent(const std::string& shp){
        std::unique_ptr<GDALDataset, gdal_dataset_deleter> dataset(
                (GDALDataset *) GDALOpenEx(U8TEXT(shp).c_str(),
                                           GDAL_OF_VECTOR,
                                           nullptr,
                                           nullptr,
                                           nullptr));
        if(dataset == nullptr){
            throw cgt_exception("could not open " + shp);
        }

        OGRLayer *layer = dataset->GetLayer(0);
        std::unique_ptr<OGRFeature, ogr_feature_deleter> feature(layer->GetFeature(0));
        if (feature == nullptr) {
            throw cgt_exception("could find geom feature in " + shp);
        }
        geometry_.reset(feature->GetGeometryRef()->clone());

        osg_modeldata modeldata;
        if (layer->GetSpatialRef() == nullptr) {
            throw cgt_exception("could find spartial information in " + shp);
        }
        char *target_wktsrs = nullptr;
        layer->GetSpatialRef()->exportToWkt(&target_wktsrs);
        modeldata.set_srs(target_wktsrs);
        proj_ = std::make_unique<cgt_proj>(source_metadata_, modeldata);
        CPLFree(target_wktsrs);
    }

    bool osg_export::is_intersect(const osg::Node& node){
        const osg::BoundingSphere bound = node.getBound();
        double x = bound.center().x();
        double y = bound.center().y();
        double z = bound.center().z();
        double r = bound.radius() / 2;

        OGRPolygon polygon;
        OGRLinearRing* ring = new OGRLinearRing; // polygon owner
        ring->addPoint(x - r, y + r, z); // left up
        ring->addPoint(x + r, y + r, z); // right up
        ring->addPoint(x + r, y - r, z); // right down
        ring->addPoint(x - r, y - r, z); // left down
        ring->addPoint(x - r, y + r, z); // left up
        polygon.addRingDirectly(ring);
        proj_->transform(&polygon);
        return geometry_->Intersects(&polygon);
    }

    bool osg_export::root_process(cgt_proj &proj, const std::string &tile_path) {
        auto node = osgDB::readRefNodeFile(U8TEXT(tile_path));
        if (node) {
            if (outer_cut) {
                if (is_intersect(*node)) {
                    tiles.push_back(get_data_name(tile_path) + "/" + get_root_name(tile_path));
                    return true;
                }
            }
            else {
                if (!is_intersect(*node)) {
                    tiles.push_back(get_data_name(tile_path) + "/" + get_root_name(tile_path));
                    return true;
                }    
            }
        }
        return false;
    }

    bool osg_export::tile_process(cgt_proj &proj, const std::string &tile_path) {
        return true;
    }

    bool osg_export::end_process() {
        spdlog::info("=========================export tile name==================================");
        for (const auto &name: tiles) {
            spdlog::info(name);
        }
        spdlog::info("===========================================================================");
        if (is_copy_) {
            spdlog::info("=================== copy export tile name==================================");
            for (const auto &name: tiles) {
                fs::path from = fs::path(source_dir_) / name;
                fs::path to = fs::path(target_dir_) / name;
                if (!fs::exists(to))
                    fs::create_directories(to);
                spdlog::info("copy {}", name);
                fs::copy(from, to, fs::copy_options::overwrite_existing);
            }
            spdlog::info("===========================================================================");
            source_metadata_.write(target_dir_ + "/metadata.xml");
        }

        return true;
    }

    void osg_export::set_outer_flag(const bool& flag) {
        outer_cut = flag;
    }
}
