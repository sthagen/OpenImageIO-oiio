// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO


#pragma once

#include <memory>

#include <OpenImageIO/export.h>
#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/string_view.h>


OIIO_NAMESPACE_BEGIN

/// Quick structure that describes a filter.
///
class OIIO_UTIL_API FilterDesc {
public:
    const char* name;  ///< name of the filter
    int dim;           ///< dimensionality: 1 or 2
    float width;       ///< Recommended width or window
    bool fixedwidth;   ///< Is the width the only one that makes sense?
    bool scalable;     ///< Is it scalable (otherwise, the width is a window)?
    bool separable;    ///< Is it separable?  (only matters if dim==2)
};



/// Filter1D is the abstract data type for a 1D filter.
/// The filters are NOT expected to have their results normalized.
class OIIO_UTIL_API Filter1D {
public:
    /// Alias for a shared pointer to a filter
    using ref = std::shared_ptr<const Filter1D>;

    Filter1D(float width)
        : m_w(width)
    {
    }
    virtual ~Filter1D(void) {};

    /// Get the width of the filter
    float width(void) const { return m_w; }

    /// Evaluate the filter at an x position (relative to filter center)
    virtual float operator()(float x) const = 0;

    /// Return the name of the filter, e.g., "box", "gaussian"
    virtual string_view name(void) const = 0;

    /// This static function allocates specific filter implementation for the
    /// name you provide and returns it as a shared_ptr. It will be
    /// automatically deleted when the last reference to it is released.
    /// Example use:
    ///        auto myfilt = Filter1D::create_shared("box", 1.0f);
    /// If the name is not recognized, return an empty shared_ptr.
    static ref create_shared(string_view filtername, float width);

    /// This static function allocates and returns an instance of the
    /// specific filter implementation for the name you provide.
    /// Example use:
    ///        Filter1D *myfilt = Filter1D::create ("box", 1.0f);
    /// The caller is responsible for deleting it when it's done.
    /// If the name is not recognized, return NULL.
    ///
    /// Note that the create_shared method is preferred over create().
    static Filter1D* create(string_view filtername, float width);

    /// Destroy a filter that was created with create().
    static void destroy(Filter1D* filt);

    /// Get the number of filters supported.
    static int num_filters();
    /// Get the info for a particular index (0..num_filters()-1).
    static void get_filterdesc(int filternum, FilterDesc* filterdesc);
    static const FilterDesc& get_filterdesc(int filternum);

protected:
    const float m_w;
};



/// Filter2D is the abstract data type for a 2D filter.
/// The filters are NOT expected to have their results normalized.
class OIIO_UTIL_API Filter2D {
public:
    /// Alias for a shared pointer to a filter
    using ref = std::shared_ptr<const Filter2D>;

    Filter2D(float width, float height)
        : m_w(width)
        , m_h(height)
    {
    }
    virtual ~Filter2D(void) {};

    /// Get the width of the filter
    float width(void) const { return m_w; }
    /// Get the height of the filter
    float height(void) const { return m_h; }

    /// Is the filter separable?
    ///
    virtual bool separable() const { return false; }

    /// Evaluate the filter at an x and y position (relative to filter
    /// center).
    virtual float operator()(float x, float y) const = 0;

    /// Evaluate just the horizontal filter (if separable; for non-separable
    /// it just evaluates at (x,0).
    virtual float xfilt(float x) const { return (*this)(x, 0.0f); }

    /// Evaluate just the vertical filter (if separable; for non-separable
    /// it just evaluates at (0,y).
    virtual float yfilt(float y) const { return (*this)(0.0f, y); }

    /// Return the name of the filter, e.g., "box", "gaussian"
    virtual string_view name(void) const = 0;

    /// This static function allocates specific filter implementation for the
    /// name you provide and returns it as a shared_ptr. It will be
    /// automatically deleted when the last reference do it is released.
    /// Example use:
    ///        auto myfilt = Filter2D::create_shared("box", 1.0f, 1.0f);
    /// If the name is not recognized, return an empty shared_ptr.
    static ref create_shared(string_view filtername, float width, float height);

    /// This static function allocates and returns an instance of the
    /// specific filter implementation for the name you provide.
    /// Example use:
    ///        Filter2D *myfilt = Filter2D::create ("box", 1.0f, 1.0f);
    /// The caller is responsible for deleting it when it's done.
    /// If the name is not recognized, return NULL.
    ///
    /// Note that the create_shared method is preferred over create().
    static Filter2D* create(string_view filtername, float width, float height);

    /// Destroy a filter that was created with create().
    static void destroy(Filter2D* filt);

    /// Don't destroy a filter -- used for non-owning shared_ptr.
    static void no_destroy(const Filter2D*) {}

    /// Get the number of filters supported.
    static int num_filters();
    /// Get the info for a particular index (0..num_filters()-1).
    static void get_filterdesc(int filternum, FilterDesc* filterdesc);
    static const FilterDesc& get_filterdesc(int filternum);

protected:
    const float m_w, m_h;
};


OIIO_NAMESPACE_END
