#include <cgt/cgttransform.h>
#include <cgt/cgtexport.h>
#include <cgt/config.h>
#include <CLI11/CLI11.hpp>

#include <string>

int main(int argc, char *argv[]) {
    static scially::gdal_init gdal_init_;

    CLI::App app{"cgt(by hwang@126.com)"};
    std::string in_location;
    std::string out_location;
    std::string target_srs;
    std::string source_srs;
    std::string source_srs_origin = "0,0,0";
    std::string target_srs_origin = "0,0,0";
    std::string shapefile;
    bool is_copy = true;
    uint32_t thread = 0;

    app.set_version_flag("-v,--version", CGT_VERSION);
    auto transform_app = app.add_subcommand("transform", "coordinate transform");
    auto export_app = app.add_subcommand("export", "export osgb data");
    app.require_subcommand();

    app
            .add_option("-i,--in", in_location, "osg Data path")
            ->check(CLI::ExistingDirectory)
            ->required(true);
    app
            .add_option("-o,--out", out_location, "output location")
            ->required(true);
    app
        .add_option("--source-srs", source_srs, "source srs")
        ->required(false);
    app
        .add_option("--source-origin", source_srs_origin, "source srs origin (default: 0,0,0)")
        ->required(false);
    app
        .add_option("-t,--thread", thread, "thread")
        ->required(false);

    transform_app
            ->add_option("--target-srs", target_srs, "target srs")
            ->required(true);
    transform_app
            ->add_option("--target-origin", target_srs_origin, "target srs origin(default: automatically calculated based on source_srs_orign)")
            ->required(false);
    export_app
            ->add_option("-s,--shapefile", shapefile, "export extent")
            ->check(CLI::ExistingFile)
            ->required(true);
    export_app
            ->add_option("-c,--copy", is_copy, "copy tile(default: true)")
            ->required(false);

    CLI11_PARSE(app, argc, argv);

    scially::osg_modeldata source_modeldata;
    if (source_srs.empty()) {
        try {
            source_modeldata.load_from_dir(in_location);
        } catch (scially::cgt_exception &e) {
            spdlog::error(e.what());
            return -1;
        }
    } else {
        source_modeldata.load(source_srs, source_srs_origin);
    }

    if(transform_app->parsed()) {
        CLI::Option* t_option = transform_app->get_option("--target-origin");
        scially::osg_modeldata target_modeldata;
        target_modeldata.load(target_srs, target_srs_origin);

        scially::osg_transform transform(in_location, out_location);
        transform.set_source_metadata(source_modeldata);
        transform.set_target_metadata(target_modeldata);
        transform.set_orign_flag(t_option->empty());
        try {
            transform.run(thread);
        } catch (scially::cgt_exception &e) {
            spdlog::error(e.what());
            return -1;
        }
        return 0;
    }

    if(export_app->parsed()) {
        scially::osg_export osg_export(in_location, out_location);
        osg_export.set_source_metadata(source_modeldata);
        try {
            osg_export.set_extent(shapefile);
            osg_export.run(thread);
        } catch (scially::cgt_exception &e) {
            spdlog::error(e.what());
            return -1;
        }
        return 0;
    }

    return -1;
}
