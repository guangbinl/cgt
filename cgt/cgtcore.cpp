//
// Created by adminstrator on 2022/6/1.
//

#include <cgt/cgtcore.h>
#include <cgt/cgtproj.h>
#include <cgt/cgtvisitor.h>

#include <BS_thread_pool/BS_thread_pool.hpp>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osg/CoordinateSystemNode>
#include <osg/ComputeBoundsVisitor>
#include <osg/Math>

#include <filesystem>
#include <cmath>
#include <chrono>

namespace scially {
    namespace fs = std::filesystem;

	void node_operator::read(const std::string& path) {
        node_name_ = fs::path(path).stem().string();
        node_ = osgDB::readRefNodeFile(U8TEXT(path));
	}

	void node_operator::write(const std::string& dir) {
        std::string tile_outpath = (fs::path(dir) / node_name_ ).string() + ".osgb";
        osg::ref_ptr<osgDB::Options> options = new osgDB::Options;
        //options->setOptionString("Compressor=zlib");
        options->setOptionString("WriteImageHint=IncludeFile Compressor=zlib");
        osgDB::writeNodeFile(*node_, U8TEXT(tile_outpath), options);
	}

	void node_operator::apply(const std::string &base_path, const vec3d_transform& algorithm) {
        geom_visitor visitor(base_path, algorithm);
        node_->accept(visitor);
        node_->dirtyBound();
	}

    osg::BoundingBox osg_base::detect_target_metadata(){
        osg::BoundingBox tile_bbox;
        tile_bbox.init();
        for(const auto &tile_dir : fs::directory_iterator(source_dir_,
                                                          ~fs::directory_options::follow_directory_symlink
                                                          | fs::directory_options::skip_permission_denied)) {
            if(!fs::is_directory(tile_dir.path()))
                continue;
            fs::path tile_name = tile_dir.path().filename();
            std::string tile_path = (tile_dir.path() / tile_name).string() + ".osgb";
            osg::ref_ptr<osg::Node> root = osgDB::readRefNodeFile(U8TEXT(tile_path));
            osg::ComputeBoundsVisitor cbv;
            root->accept(cbv);
            osg::BoundingBox bb = cbv.getBoundingBox(); // in local coords.
            tile_bbox.expandBy(bb);
        }
        return tile_bbox;
    }

    void osg_base::run(uint32_t max_thread) {
        auto start = std::chrono::high_resolution_clock::now();

        max_thread = max_thread == 0 ?  std::thread::hardware_concurrency() - 1 : 1;
        spdlog::info("cgt set thread to {}", max_thread);

        BS::thread_pool thread_pool(max_thread);

        cgt_proj proj_(source_metadata_, target_metadata_, calculate_orign);

        for (const auto &data_x_dir: fs::directory_iterator(source_dir_,
                                                            ~fs::directory_options::follow_directory_symlink
                                                            | fs::directory_options::skip_permission_denied)) {
            if (!fs::is_directory(data_x_dir.path()))
                continue;

            std::string data_x_name = data_x_dir.path().stem().string();

            for (const auto &data_dir: fs::directory_iterator(data_x_dir.path(),
                                                              ~fs::directory_options::follow_directory_symlink
                                                              | fs::directory_options::skip_permission_denied)) {
                if (!fs::is_directory(data_dir.path()))
                    continue;

                std::string tile_name = data_dir.path().stem().string();
                std::string tile_path = (data_dir.path() / (tile_name + ".osgb")).string();
                spdlog::info("find data {}/{}, start process tile", data_x_name, tile_name);
                if (!root_process(proj_, tile_path))
                    continue;

                std::string tile_dir = data_dir.path().string();
                std::string out_dir = (fs::path(target_dir_) / data_x_name / fs::path(tile_name)).string();

                if (!fs::exists(out_dir)) {
                    fs::create_directories(out_dir);
                }

                thread_pool.push_task([this, &proj_, tile_dir, out_dir]() {
                    for (const auto &tile_path: fs::directory_iterator(tile_dir,
                                                                       ~fs::directory_options::follow_directory_symlink
                                                                       |
                                                                       fs::directory_options::skip_permission_denied)) {
                        if (fs::is_directory(tile_path.path()))
                            continue;
                        tile_process(proj_, tile_path.path().string());
                    }
                });
            }
        }

        thread_pool.wait_for_tasks();
        end_process();
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        spdlog::info("Processing is completed, it takes {}s", duration.count() / 1000.0);
    }

    std::string osg_base::get_root_name(const std::string &tile_path) {
        return fs::path(tile_path).parent_path().stem().string();
    }

    std::string osg_base::get_data_name(const std::string &tile_path) {
        return fs::path(tile_path).parent_path().parent_path().stem().string();
    }
}