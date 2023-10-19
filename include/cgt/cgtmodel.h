#pragma once

#include <cgt/cgtexception.h>
#include <osg/Vec3d>

#include <vector>
#include <cstdint>
#include <string>
#include <sstream>

namespace scially {
    CGTLIBRARY std::vector<std::string> split(const std::string &str, const std::string &delimiters);

    class CGTLIBRARY osg_modeldata {
    public:
        void load(const std::string &srs, const std::string &srs_origin);

        void load_from_dir(const std::string &input);

        void load_from_file(const std::string &input);

        void write(const std::string &output);

        std::string srs() const noexcept { return srs_; }

        void set_srs(const std::string &srs) noexcept { srs_ = srs; }
        osg::Vec3d origin() const noexcept { return origin_; }
        void set_origin(osg::Vec3d origin) noexcept { origin_ = origin;}

        bool is_valid() const noexcept { return !srs_.empty(); }

    private:
        osg::Vec3d parse_origin(const std::string& srs_origin) const noexcept;

        // GZ2000 => EPSG:0
        // ENU: lat, lnt, h => topocentric
        std::string srs_;
        osg::Vec3d origin_;
    };
}
