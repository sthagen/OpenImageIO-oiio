// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

#pragma once

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/ustring.h>


// Define symbols that let client applications determine if newly added
// features are supported.

// Is the close() method present?
#define OIIO_IMAGECACHE_SUPPORTS_CLOSE 1

// Is the getattributetype() method present? (Added in 2.5)
#define OIIO_IMAGECACHE_SUPPORTS_GETATTRIBUTETYPE 1

// Does invalidate() support the optional `force` flag?
#define OIIO_IMAGECACHE_INVALIDATE_FORCE 1

// Does ImageCache::create() return a shared pointer?
#define OIIO_IMAGECACHE_CREATE_SHARED 1



OIIO_NAMESPACE_BEGIN

// Forward declarations
class TextureOpt_v2;

class ImageCachePerThreadInfo;
class ImageCacheFile;
class ImageCacheTile;
class ImageCacheImpl;



/// Define an API to an abstract class that manages image files,
/// caches of open file handles as well as tiles of pixels so that truly
/// huge amounts of image data may be accessed by an application with low
/// memory footprint.
class OIIO_API ImageCache {
public:
    /// @{
    /// @name Creating and destroying an image cache
    ///
    /// ImageCache is an abstract API described as a pure virtual class.
    /// The actual internal implementation is not exposed through the
    /// external API of OpenImageIO.  Because of this, you cannot construct
    /// or destroy the concrete implementation, so two static methods of
    /// ImageCache are provided:

    /// Create a ImageCache and return a shared pointer to it.
    ///
    /// @param  shared
    ///     If `true`, the pointer returned will be a shared ImageCache (so
    ///     that multiple parts of an application that request an ImageCache
    ///     will all end up with the same one). If `shared` is `false`, a
    ///     completely unique ImageCache will be created and returned.
    ///
    /// @returns
    ///     A shared pointer to an ImageCache, which can only be freed with
    ///     `ImageCache::destroy()`.
    ///
    /// @see    ImageCache::destroy
    static std::shared_ptr<ImageCache> create(bool shared = true);

    /// Release the shared_ptr to an ImageCache, including freeing all
    /// system resources that it holds if no one else is still using it. This
    /// is not strictly necessary to call, simply destroying the shared_ptr
    /// will do the same thing, but this call is for backward compatibility
    /// and is helpful if you want to use the teardown option.
    ///
    /// @param  cache
    ///     Shared pointer to the ImageCache to destroy.
    ///
    /// @param  teardown
    ///     For a shared ImageCache, if the `teardown` parameter is
    ///     `true`, it will try to truly destroy the shared cache if
    ///     nobody else is still holding a reference (otherwise, it will
    ///     leave it intact). This parameter has no effect if `cache` was
    ///     not the single globally shared ImageCache.
    static void destroy(std::shared_ptr<ImageCache>& cache,
                        bool teardown = false);

    /// @}


    /// @{
    /// @name Setting options and limits for the image cache
    ///
    /// These are the list of attributes that can bet set or queried by
    /// attribute/getattribute:
    ///
    /// - `int max_open_files` :
    ///           The approximate maximum number of file handles that the
    ///           image cache will hold open simultaneously. This is not an
    ///           iron-clad guarantee; the number of handles may momentarily
    ///           exceed this by a small percentage. (Default = 100)
    /// - `float max_memory_MB` :
    ///           The approximate maximum amount of memory (measured in MB)
    ///           used for the internal "tile cache." (Default: 1024.0 MB)
    /// - `string searchpath` :
    ///           The search path for images: a colon-separated list of
    ///           directories that will be searched in order for any image
    ///           filename that is not specified as an absolute path.
    ///           (Default: "")
    /// - `string plugin_searchpath` :
    ///           The search path for plugins: a colon-separated list of
    ///           directories that will be searched in order for any OIIO
    ///           plugins, if not found in OIIO's `lib` directory.
    ///           (Default: "")
    /// - `int autotile` ,
    ///   `int autoscanline` :
    ///           These attributes control how the image cache deals with
    ///           images that are not "tiled" (i.e., are stored as
    ///           scanlines).
    ///
    ///           If `autotile` is set to 0 (the default), an untiled image
    ///           will be treated as if it were a single tile of the
    ///           resolution of the whole image.  This is simple and fast,
    ///           but can lead to poor cache behavior if you are
    ///           simultaneously accessing many large untiled images.
    ///
    ///           If `autotile` is nonzero (e.g., 64 is a good recommended
    ///           value), any untiled images will be read and cached as if
    ///           they were constructed in tiles of size:
    ///
    ///           - `autotile * autotile`
    ///                 if `autoscanline` is 0
    ///           - `width * autotile`
    ///                 if `autoscanline` is nonzero.
    ///
    ///           In both cases, this should lead more efficient caching.
    ///           The `autoscanline` determines whether the "virtual tiles"
    ///           in the cache are square (if `autoscanline` is 0, the
    ///           default) or if they will be as wide as the image (but only
    ///           `autotile` scanlines high).  You should try in your
    ///           application to see which leads to higher performance.
    /// - `int autoscanline` :
    ///           autotile using full width tiles
    /// - `int automip` :
    ///           If 0 (the default), an untiled single-subimage file will
    ///           only be able to utilize that single subimage.
    ///           If nonzero, any untiled, single-subimage (un-MIP-mapped)
    ///           images will have lower-resolution MIP-map levels generated
    ///           on-demand if pixels are requested from the lower-res
    ///           subimages (that don't really exist). Essentially this
    ///           makes the ImageCache pretend that the file is MIP-mapped
    ///           even if it isn't.
    /// - `int accept_untiled` :
    ///           When nonzero, ImageCache accepts untiled images as usual.
    ///           When zero, ImageCache will reject untiled images with an
    ///           error condition, as if the file could not be properly
    ///           read. This is sometimes helpful for applications that want
    ///           to enforce use of tiled images only. (default=1)
    /// - `int accept_unmipped` :
    ///           When nonzero, ImageCache accepts un-MIPmapped images as
    ///           usual.  When set to zero, ImageCache will reject
    ///           un-MIPmapped images with an error condition, as if the
    ///           file could not be properly read. This is sometimes helpful
    ///           for applications that want to enforce use of MIP-mapped
    ///           images only. (Default: 1)
    /// - `int statistics:level` :
    ///           verbosity of statistics auto-printed.
    /// - `int forcefloat` :
    ///           If set to nonzero, all image tiles will be converted to
    ///           `float` type when stored in the image cache.  This can be
    ///           helpful especially for users of ImageBuf who want to
    ///           simplify their image manipulations to only need to
    ///           consider `float` data. The default is zero, meaning that
    ///           image pixels are not forced to be `float` when in cache.
    /// - `int failure_retries` :
    ///           When an image file is opened or a tile/scanline is read but
    ///           a file error occurs, if this attribute is nonzero, it will
    ///           try the operation again up to this many times before giving
    ///           up and reporting a failure. Setting this to a small nonzero
    ///           number (like 3) may help make an application more robust to
    ///           occasional spurious networking or other glitches that would
    ///           otherwise cause the entire long-running application to fail
    ///           upon a single transient error. (Default: 0)
    /// - `int deduplicate` :
    ///           When nonzero, the ImageCache will notice duplicate images
    ///           under different names if their headers contain a SHA-1
    ///           fingerprint (as is done with `maketx`-produced textures)
    ///           and handle them more efficiently by avoiding redundant
    ///           reads.  The default is 1 (de-duplication turned on). The
    ///           only reason to set it to 0 is if you specifically want to
    ///           disable the de-duplication optimization.
    /// - `int max_open_files_strict` :
    ///             If nonzero, work harder to make sure that we have
    ///             smaller possible overages to the max open files limit.
    ///             (Default: 0)
    /// - `string substitute_image` :
    ///           When set to anything other than the empty string, the
    ///           ImageCache will use the named image in place of *all*
    ///           other images.  This allows you to run an app using OIIO
    ///           and (if you can manage to get this option set)
    ///           automagically substitute a grid, zone plate, or other
    ///           special debugging image for all image/texture use.
    /// - `int unassociatedalpha` :
    ///           When nonzero, will request that image format readers try
    ///           to leave input images with unassociated alpha as they are,
    ///           rather than automatically converting to associated alpha
    ///           upon reading the pixels.  The default is 0, meaning that
    ///           the automatic conversion will take place.
    /// - `int max_errors_per_file` :
    ///           The maximum number of errors that will be printed for each
    ///           file. The default is 100. If your output is cluttered with
    ///           error messages and after the first few for each file you
    ///           aren't getting any helpful additional information, this
    ///           can cut down on the clutter and the runtime. (default:
    ///           100)
    /// - `int trust_file_extensions` :
    ///           When nonzero, assume that the file extensions of any
    ///           texture requests correctly indicates the file format (when
    ///           enabled, this reduces the number of file opens, at the
    ///           expense of not being able to open files if their format do
    ///           not actually match their filename extension). Default: 0
    /// - `string colorspace` :
    ///           The working colorspace of the texture system. Default: none.
    /// - `string colorconfig` :
    ///           Name of the OCIO config to use. Default: "" (meaning to use
    ///           the default color config).
    ///
    /// - `string options`
    ///           This catch-all is simply a comma-separated list of
    ///           `name=value` settings of named options, which will be
    ///           parsed and individually set. Example:
    ///
    ///                ic->attribute ("options", "max_memory_MB=512.0,autotile=1");
    ///
    ///           Note that if an option takes a string value that must
    ///           itself contain a comma, it is permissible to enclose the
    ///           value in either single (` ' ' `) or double (` " " `) quotes.
    ///
    /// **Read-only attributes**
    ///
    /// Additionally, there are some read-only attributes that can be
    /// queried with `getattribute()` even though they cannot be set via
    /// `attribute()`:
    ///
    /// - `int total_files` :
    ///           The total number of unique file names referenced by calls
    ///           to the ImageCache.
    ///
    /// - `string[] all_filenames` :
    ///           An array that will be filled with the list of the names of
    ///           all files referenced by calls to the ImageCache. (The
    ///           array is of `ustring` or `char*`.)
    ///
    /// - `int64 stat:cache_footprint` :
    ///           Total bytes used by image cache.
    /// - `int64 stat:cache_memory_used` :
    ///           Total bytes used by tile cache.
    ///
    /// - `int stat:tiles_created` ,
    ///   `int stat:tiles_current` ,
    ///   `int stat:tiles_peak` :
    ///           Total times created, still allocated (at the time of the
    ///           query), and the peak number of tiles in memory at any
    ///           time.
    ///
    /// - `int stat:open_files_created` ,
    ///   `int stat:open_files_current` ,
    ///   `int stat:open_files_peak` :
    ///           Total number of times a file was opened, number still
    ///           opened (at the time of the query), and the peak number of
    ///           files opened at any time.
    ///
    /// - `int stat:find_tile_calls` :
    ///           Number of times a filename was looked up in the file cache.
    ///
    /// - `int64 stat:image_size` :
    ///           Total size (uncompressed bytes of pixel data) of all
    ///           images referenced by the ImageCache. (Note: Prior to 1.7,
    ///           this was called `stat:files_totalsize`.)
    ///
    /// - `int64 stat:file_size` :
    ///           Total size of all files (as on disk, possibly compressed)
    ///           of all images referenced by the ImageCache.
    ///
    /// - `int64 stat:bytes_read` :
    ///           Total size (uncompressed bytes of pixel data) read.
    ///
    /// - `int stat:unique_files` :
    ///           Number of unique files opened.
    ///
    /// - `float stat:fileio_time` :
    ///           Total I/O-related time (seconds).
    ///
    /// - `float stat:fileopen_time` :
    ///           I/O time related to opening and reading headers (but not
    ///           pixel I/O).
    ///
    /// - `float stat:file_locking_time` :
    ///           Total time (across all threads) that threads blocked
    ///           waiting for access to the file data structures.
    ///
    /// - `float stat:tile_locking_time` :
    ///           Total time (across all threads) that threads blocked
    ///           waiting for access to the tile cache data structures.
    ///
    /// - `float stat:find_file_time` :
    ///           Total time (across all threads) that threads spent looking
    ///           up files by name.
    ///
    /// - `float stat:find_tile_time` :
    ///           Total time (across all threads) that threads spent looking
    ///           up individual tiles.
    ///
    /// The following member functions of ImageCache allow you to set (and
    /// in some cases retrieve) options that control the overall behavior of
    /// the image cache:

    /// Set a named attribute (i.e., a property or option) of the
    /// ImageCache.
    ///
    /// Example:
    ///
    ///     ImageCache *ic;
    ///     ...
    ///     int maxfiles = 50;
    ///     ic->attribute ("max_open_files", TypeDesc::INT, &maxfiles);
    ///
    ///     const char *path = "/my/path";
    ///     ic->attribute ("searchpath", TypeDesc::STRING, &path);
    ///
    ///     // There are specialized versions for setting a single int,
    ///     // float, or string without needing types or pointers:
    ///     ic->attribute ("max_open_files", 50);
    ///     ic->attribute ("max_memory_MB", 4000.0f);
    ///     ic->attribute ("searchpath", "/my/path");
    ///
    /// Note: When passing a string, you need to pass a pointer to the
    /// `char*`, not a pointer to the first character.  (Rationale: for an
    /// `int` attribute, you pass the address of the `int`.  So for a
    /// string, which is a `char*`, you need to pass the address of the
    /// string, i.e., a `char**`).
    ///
    /// @param  name    Name of the attribute to set.
    /// @param  type    TypeDesc describing the type of the attribute.
    /// @param  val     Pointer to the value data.
    /// @returns        `true` if the name and type were recognized and the
    ///                 attribute was set, or `false` upon failure
    ///                 (including it being an unrecognized attribute or not
    ///                 of the correct type).
    ///
    bool attribute(string_view name, TypeDesc type, const void* val);

    /// Specialized `attribute()` for setting a single `int` value.
    bool attribute(string_view name, int val)
    {
        return attribute(name, TypeInt, &val);
    }
    /// Specialized `attribute()` for setting a single `float` value.
    bool attribute(string_view name, float val)
    {
        return attribute(name, TypeFloat, &val);
    }
    bool attribute(string_view name, double val)
    {
        float f = (float)val;
        return attribute(name, TypeFloat, &f);
    }
    /// Specialized `attribute()` for setting a single string value.
    bool attribute(string_view name, string_view val)
    {
        std::string valstr(val);
        const char* s = valstr.c_str();
        return attribute(name, TypeString, &s);
    }

    /// Get the named attribute, store it in `*val`. All of the attributes
    /// that may be set with the `attribute() call` may also be queried with
    /// `getattribute()`.
    ///
    /// Examples:
    ///
    ///     ImageCache *ic;
    ///     ...
    ///     int maxfiles;
    ///     ic->getattribute ("max_open_files", TypeDesc::INT, &maxfiles);
    ///
    ///     const char *path;
    ///     ic->getattribute ("searchpath", TypeDesc::STRING, &path);
    ///
    ///     // There are specialized versions for retrieving a single int,
    ///     // float, or string without needing types or pointers:
    ///     int maxfiles;
    ///     ic->getattribute ("max_open_files", maxfiles);
    ///     const char *path;
    ///     ic->getattribute ("searchpath", &path);
    ///
    /// Note: When retrieving a string, you need to pass a pointer to the
    /// `char*`, not a pointer to the first character. Also, the `char*`
    /// will end up pointing to characters owned by the ImageCache; the
    /// caller does not need to ever free the memory that contains the
    /// characters.
    ///
    /// @param  name    Name of the attribute to retrieve.
    /// @param  type    TypeDesc describing the type of the attribute.
    /// @param  val     Pointer where the attribute value should be stored.
    /// @returns        `true` if the name and type were recognized and the
    ///                 attribute was retrieved, or `false` upon failure
    ///                 (including it being an unrecognized attribute or not
    ///                 of the correct type).
    bool getattribute(string_view name, TypeDesc type, void* val) const;

    /// Specialized `attribute()` for retrieving a single `int` value.
    bool getattribute(string_view name, int& val) const
    {
        return getattribute(name, TypeInt, &val);
    }
    /// Specialized `attribute()` for retrieving a single `float` value.
    bool getattribute(string_view name, float& val) const
    {
        return getattribute(name, TypeFloat, &val);
    }
    bool getattribute(string_view name, double& val) const
    {
        float f;
        bool ok = getattribute(name, TypeFloat, &f);
        if (ok)
            val = f;
        return ok;
    }
    /// Specialized `attribute()` for retrieving a single `string` value
    /// as a `char*`.
    bool getattribute(string_view name, char** val) const
    {
        return getattribute(name, TypeString, val);
    }
    /// Specialized `attribute()` for retrieving a single `string` value
    /// as a `std::string`.
    bool getattribute(string_view name, std::string& val) const
    {
        ustring s;
        bool ok = getattribute(name, TypeString, &s);
        if (ok)
            val = s.string();
        return ok;
    }

    /// If the named attribute is known, return its data type. If no such
    /// attribute exists, return `TypeUnknown`.
    ///
    /// This was added in version 2.5.
    TypeDesc getattributetype(string_view name) const;

    /// @}


    /// @{
    /// @name Opaque data for performance lookups
    ///
    /// The ImageCache implementation needs to maintain certain per-thread
    /// state, and some methods take an opaque `Perthread` pointer to this
    /// record. There are three options for how to deal with it:
    ///
    /// 1. Don't worry about it at all: don't use the methods that want
    ///    `Perthread` pointers, or always pass `nullptr` for any
    ///    `Perthread*1 arguments, and ImageCache will do
    ///    thread-specific-pointer retrieval as necessary (though at some
    ///    small cost).
    ///
    /// 2. If your app already stores per-thread information of its own, you
    ///    may call `get_perthread_info(nullptr)` to retrieve it for that
    ///    thread, and then pass it into the functions that allow it (thus
    ///    sparing them the need and expense of retrieving the
    ///    thread-specific pointer). However, it is crucial that this
    ///    pointer not be shared between multiple threads. In this case, the
    ///    ImageCache manages the storage, which will automatically be
    ///    released when the thread terminates.
    ///
    /// 3. If your app also wants to manage the storage of the `Perthread`,
    ///    it can explicitly create one with `create_perthread_info()`, pass
    ///    it around, and eventually be responsible for destroying it with
    ///    `destroy_perthread_info()`. When managing the storage, the app
    ///    may reuse the `Perthread` for another thread after the first is
    ///    terminated, but still may not use the same `Perthread` for two
    ///    threads running concurrently.

    /// Define an opaque data type that allows us to have a pointer to
    /// certain per-thread information that the ImageCache maintains. Any
    /// given one of these should NEVER be shared between running threads.
    using Perthread = ImageCachePerThreadInfo;

    /// Retrieve a Perthread, unique to the calling thread. This is a
    /// thread-specific pointer that will always return the Perthread for a
    /// thread, which will also be automatically destroyed when the thread
    /// terminates.
    ///
    /// Applications that want to manage their own Perthread pointers (with
    /// `create_thread_info` and `destroy_thread_info`) should still call
    /// this, but passing in their managed pointer. If the passed-in
    /// thread_info is not NULL, it won't create a new one or retrieve a
    /// TSP, but it will do other necessary housekeeping on the Perthread
    /// information.
    Perthread* get_perthread_info(Perthread* thread_info = NULL);

    /// Create a new Perthread. It is the caller's responsibility to
    /// eventually destroy it using `destroy_thread_info()`.
    Perthread* create_thread_info();

    /// Destroy a Perthread that was allocated by `create_thread_info()`.
    void destroy_thread_info(Perthread* thread_info);

    /// Define an opaque data type that allows us to have a handle to an
    /// image (already having its name resolved) but without exposing any
    /// internals.
    using ImageHandle = ImageCacheFile;

    /// Retrieve an opaque handle for fast texture lookups, or nullptr upon
    /// failure.  The filename is presumed to be UTF-8 encoded. The `options`,
    /// if not null, may be used to create a separate handle for certain
    /// texture option choices. (Currently unused, but reserved for the future
    /// or for alternate IC implementations.) The opaque pointer `thread_info`
    /// is thread-specific information returned by `get_perthread_info()`.
    ImageHandle* get_image_handle(ustring filename,
                                  Perthread* thread_info       = nullptr,
                                  const TextureOpt_v2* options = nullptr);

    /// Get an ImageHandle using a UTF-16 encoded wstring filename.
    ImageHandle* get_image_handle(const std::wstring& filename,
                                  Perthread* thread_info       = nullptr,
                                  const TextureOpt_v2* options = nullptr)
    {
        return get_image_handle(ustring(Strutil::utf16_to_utf8(filename)),
                                thread_info, options);
    }

    /// Return true if the image handle (previously returned by
    /// `get_image_handle()`) is a valid image that can be subsequently read.
    bool good(ImageHandle* file);

    /// Given a handle, return the filename for that image.
    ///
    /// This method was added in OpenImageIO 2.3.
    ustring filename_from_handle(ImageHandle* handle);

    /// @}


    /// @{
    /// @name   Getting information about images
    ///

    /// Given possibly-relative `filename` (UTF-8 encoded), resolve it and use
    /// the true path to the file, with searchpath logic applied.
    std::string resolve_filename(const std::string& filename) const;

    /// Get information or metadata about the named image and store it in
    /// `*data`.
    ///
    /// Data names may include any of the following:
    ///
    /// - `"exists"` : Stores the value 1 (as an `int`) if the file exists and
    ///   is an image format that OpenImageIO can read, or 0 if the file
    ///   does not exist, or could not be properly read as an image. Note
    ///   that unlike all other queries, this query will "succeed" (return
    ///   `true`) even if the file does not exist.
    ///
    /// - `"udim"` : Stores the value 1 (as an `int`) if the file is a
    ///   "virtual UDIM" or texture atlas file (as described in
    ///   :ref:`sec-texturesys-udim`) or 0 otherwise.
    ///
    /// - `"subimages"` : The number of subimages in the file, as an `int`.
    ///
    /// - `"resolution"` : The resolution of the image file, which is an
    ///   array of 2 integers (described as `TypeDesc(INT,2)`).
    ///
    /// - `"miplevels"` : The number of MIPmap levels for the specified
    ///   subimage (an integer).
    ///
    /// - `"texturetype"` : A string describing the type of texture of the
    ///   given file, which describes how the texture may be used (also
    ///   which texture API call is probably the right one for it). This
    ///   currently may return one of: `"unknown"`, `"Plain Texture"`,
    ///   `"Volume Texture"`, `"Shadow"`, or `"Environment"`.
    ///
    /// - `"textureformat"` : A string describing the format of the given
    ///   file, which describes the kind of texture stored in the file. This
    ///   currently may return one of: `"unknown"`, `"Plain Texture"`,
    ///   `"Volume Texture"`, `"Shadow"`, `"CubeFace Shadow"`,
    ///   `"Volume Shadow"`, `"LatLong Environment"`, or
    ///   `"CubeFace Environment"`. Note that there are several kinds of
    ///   shadows and environment maps, all accessible through the same API
    ///   calls.
    ///
    /// - `"channels"` : The number of color channels in the file (an
    ///   `int`).
    ///
    /// - `"format"` : The native data format of the pixels in the file (an
    ///   integer, giving the `TypeDesc::BASETYPE` of the data). Note that
    ///   this is not necessarily the same as the data format stored in the
    ///   image cache.
    ///
    /// - `"cachedformat"` : The native data format of the pixels as stored
    ///   in the image cache (an integer, giving the `TypeDesc::BASETYPE` of
    ///   the data).  Note that this is not necessarily the same as the
    ///   native data format of the file.
    ///
    /// - `"datawindow"` : Returns the pixel data window of the image, which
    ///   is either an array of 4 integers (returning xmin, ymin, xmax,
    ///   ymax) or an array of 6 integers (returning xmin, ymin, zmin, xmax,
    ///   ymax, zmax). The z values may be useful for 3D/volumetric images;
    ///   for 2D images they will be 0).
    ///
    /// - `"displaywindow"` : Returns the display (a.k.a. "full") window of
    ///   the image, which is either an array of 4 integers (returning xmin,
    ///   ymin, xmax, ymax) or an array of 6 integers (returning xmin, ymin,
    ///   zmin, xmax, ymax, zmax). The z values may be useful for
    ///   3D/volumetric images; for 2D images they will be 0).
    ///
    /// - `"worldtocamera"` : The viewing matrix, which is a 4x4 matrix (an
    ///   `Imath::M44f`, described as `TypeDesc(FLOAT,MATRIX)`), giving the
    ///   world-to-camera 3D transformation matrix that was used when  the
    ///   image was created. Generally, only rendered images will have this.
    ///
    /// - `"worldtoscreen"` : The projection matrix, which is a 4x4 matrix
    ///   (an `Imath::M44f`, described as `TypeDesc(FLOAT,MATRIX)`), giving
    ///   the matrix that projected points from world space into a 2D screen
    ///   coordinate system where $x$ and $y$ range from -1 to +1.
    ///   Generally, only rendered images will have this.
    ///
    /// - `"worldtoNDC"` : The projection matrix, which is a 4x4 matrix
    ///   (an `Imath::M44f`, described as `TypeDesc(FLOAT,MATRIX)`), giving
    ///   the matrix that projected points from world space into a 2D NDC
    ///   coordinate system where $x$ and $y$ range from 0 to +1. Generally,
    ///   only rendered images will have this.
    ///
    /// - `"averagecolor"` : If available in the metadata (generally only
    ///   for files that have been processed by `maketx`), this will return
    ///   the average color of the texture (into an array of `float`).
    ///
    /// - `"averagealpha"` : If available in the metadata (generally only
    ///   for files that have been processed by `maketx`), this will return
    ///   the average alpha value of the texture (into a `float`).
    ///
    /// - `"constantcolor"` : If the metadata (generally only for files that
    ///   have been processed by `maketx`) indicates that the texture has
    ///   the same values for all pixels in the texture, this will retrieve
    ///   the constant color of the texture (into an array of floats). A
    ///   non-constant image (or one that does not have the special metadata
    ///   tag identifying it as a constant texture) will fail this query
    ///   (return `false`).
    ///
    /// - `"constantalpha"` : If the metadata indicates that the texture has
    ///   the same values for all pixels in the texture, this will retrieve
    ///   the constant alpha value of the texture (into a float). A
    ///   non-constant image (or one that does not have the special metadata
    ///   tag identifying it as a constant texture) will fail this query
    ///   (return `false`).
    ///
    /// - `"stat:tilesread"` : Number of tiles read from this file
    ///   (`int64`).
    ///
    /// - `"stat:bytesread"` : Number of bytes of uncompressed pixel data
    ///   read from this file (`int64`).
    ///
    /// - `"stat:redundant_tiles"` : Number of times a tile was read, where
    ///   the same tile had been rad before. (`int64`).
    ///
    /// - `"stat:redundant_bytesread"` : Number of bytes (of uncompressed
    ///   pixel data) in tiles that were read redundantly. (`int64`).
    ///
    /// - `"stat:redundant_bytesread"` : Number of tiles read from this file (`int`).
    ///
    /// - `"stat:image_size"` : Size of the uncompressed image pixel data
    /// of this image, in bytes (`int64`).
    ///
    /// - `"stat:file_size"` : Size of the disk file (possibly compressed)
    ///   for this image, in bytes (`int64`).
    ///
    /// - `"stat:timesopened"` : Number of times this file was opened
    ///   (`int`).
    ///
    /// - `"stat:iotime"` : Time (in seconds) spent on all I/O for this file
    ///   (`float`).
    ///
    /// - `"stat:mipsused"` : Stores 1 if any MIP levels beyond the highest
    ///   resolution were accessed, otherwise 0. (`int`)
    ///
    /// - `"stat:is_duplicate"` : Stores 1 if this file was a duplicate of
    ///   another image, otherwise 0. (`int`)
    ///
    /// - *Anything else*  : For all other data names, the the metadata of
    ///   the image file will be searched for an item that matches both the
    ///   name and data type.
    ///
    ///
    ///
    /// @param  filename
    ///             The name of the image, as a UTF-8 encoded ustring.
    /// @param  subimage/miplevel
    ///             The subimage and MIP level to query.
    /// @param  dataname
    ///             The name of the metadata to retrieve.
    /// @param  datatype
    ///             TypeDesc describing the data type.
    /// @param  data
    ///             Pointer to the caller-owned memory where the values
    ///             should be stored. It is the caller's responsibility to
    ///             ensure that `data` points to a large enough storage area
    ///             to accommodate the `datatype` requested.
    ///
    /// @returns
    ///             `true` if `get_image_info()` is able to find the
    ///             requested `dataname` for the image and it matched the
    ///             requested `datatype`.  If the requested data was not
    ///             found or was not of the right data type, return `false`.
    ///             Except for the `"exists"` query, a file that does not
    ///             exist or could not be read properly as an image also
    ///             constitutes a query failure that will return `false`.
    bool get_image_info(ustring filename, int subimage, int miplevel,
                        ustring dataname, TypeDesc datatype, void* data);
    /// A more efficient variety of `get_image_info()` for cases where you
    /// can use an `ImageHandle*` to specify the image and optionally have a
    /// `Perthread*` for the calling thread.
    bool get_image_info(ImageHandle* file, Perthread* thread_info, int subimage,
                        int miplevel, ustring dataname, TypeDesc datatype,
                        void* data);

    /// Copy the ImageSpec that describes the named image file.
    ///
    /// Note that the spec returned describes the file as it exists in the
    /// file, at the base (highest-resolution) MIP level of that subimage.
    /// Certain aspects of the in-cache representation may differ from the
    /// file (due to ImageCache implementation strategy or options like
    /// `"forcefloat"` or `"autotile"`). If you really need to know the
    /// in-cache data type, tile size, or how the resolution or tiling changes
    /// on a particular MIP level, you should use `get_cache_dimensions()`.
    ///
    /// @param  filename
    ///             The name of the image, as a UTF-8 encoded ustring.
    /// @param  spec
    ///             ImageSpec into which will be copied the spec for the
    ///             requested image.
    /// @param  subimage
    ///             The subimage to query.
    /// @returns
    ///             `true` upon success, `false` upon failure failure (such
    ///             as being unable to find, open, or read the file, or if
    ///             it does not contain the designated subimage.
    bool get_imagespec(ustring filename, ImageSpec& spec, int subimage = 0);
    /// A more efficient variety of `get_imagespec()` for cases where you
    /// can use an `ImageHandle*` to specify the image and optionally have a
    /// `Perthread*` for the calling thread.
    bool get_imagespec(ImageHandle* file, Perthread* thread_info,
                       ImageSpec& spec, int subimage = 0);

    /// DEPRECATED(3.0) old API. Note that the miplevel and native parameters
    /// are ignored: it will always get the native spec of miplevel 0. We
    /// recommend switching to the new API.
    bool get_imagespec(ustring filename, ImageSpec& spec, int subimage,
                       int miplevel, bool native = false)
    {
        return get_imagespec(filename, spec, subimage);
    }
    /// DEPRECATED(3.0) old API.
    bool get_imagespec(ImageHandle* file, Perthread* thread_info,
                       ImageSpec& spec, int subimage, int miplevel,
                       bool native = false)
    {
        return get_imagespec(file, thread_info, spec, subimage);
    }

    /// Return a pointer to an ImageSpec that describes the named image file.
    /// If the file is found and is an image format that can be read,
    /// otherwise return `nullptr`.
    ///
    /// This method is much more efficient than `get_imagespec()`, since it
    /// just returns a pointer to the spec held internally by the ImageCache
    /// (rather than copying the spec to the user's memory). However, the
    /// caller must beware that the pointer is only valid as long as nobody
    /// (even other threads) calls `invalidate()` on the file, or
    /// `invalidate_all()`, or destroys the ImageCache.
    ///
    /// Note that the spec returned describes the file as it exists in the
    /// file, at the base (highest-resolution) MIP level of that subimage.
    /// Certain aspects of the in-cache representation may differ from the
    /// file (due to ImageCache implementation strategy or options like
    /// `"forcefloat"` or `"autotile"`). If you really need to know the
    /// in-cache data type, tile size, or how the resolution or tiling changes
    /// on a particular MIP level, you should use `get_cache_dimensions()`.
    ///
    /// @param  filename
    ///             The name of the image, as a UTF-8 encoded ustring.
    /// @param  subimage
    ///             The subimage to query.
    /// @returns
    ///             A pointer to the spec, if the image is found and able to
    ///             be opened and read by an available image format plugin,
    ///             and the designated subimage exists.
    const ImageSpec* imagespec(ustring filename, int subimage = 0);
    /// A more efficient variety of `imagespec()` for cases where you can
    /// use an `ImageHandle*` to specify the image and optionally have a
    /// `Perthread*` for the calling thread.
    const ImageSpec* imagespec(ImageHandle* file, Perthread* thread_info,
                               int subimage = 0);

    /// DEPRECATED(3.0) old API. Note that the miplevel and native parameters
    /// are ignored: it will always get the native spec of miplevel 0. We
    /// recommend switching to the new API.
    const ImageSpec* imagespec(ustring filename, int subimage, int miplevel,
                               bool native = false)
    {
        return imagespec(filename, subimage);
    }
    /// DEPRECATED(3.0) old API.
    const ImageSpec* imagespec(ImageHandle* file, Perthread* thread_info,
                               int subimage, int miplevel, bool native = false)
    {
        return imagespec(file, thread_info, subimage);
    }

    /// Copy the image dimensions (x, y, z, width, height, depth, full*,
    /// nchannels, format) and data types that describes the named image
    /// cache file for the specified subimage and miplevel. It does *not*
    /// copy arbitrary named metadata or channel names (thus, for an
    /// `ImageSpec` with lots of metadata, it is much less expensive than
    /// copying the whole thing with `operator=()`). The associated
    /// metadata and channels names can be retrieved with `imagespec()`
    /// or `get_imagespec`.
    ///
    /// @param  filename
    ///             The name of the image, as a UTF-8 encoded ustring.
    /// @param  spec
    ///             ImageSpec into which will be copied the dimensions
    ///             for the requested image.
    /// @param  subimage/miplevel
    ///             The subimage and mip level to query.
    /// @returns
    ///             `true` upon success, `false` upon failure failure (such
    ///             as being unable to find, open, or read the file, or if
    ///             it does not contain the designated subimage or mip level.
    bool get_cache_dimensions(ustring filename, ImageSpec& spec,
                              int subimage = 0, int miplevel = 0);
    /// A more efficient variety of `get_cache_dimensions()` for cases where
    /// you can use an `ImageHandle*` to specify the image and optionally
    /// have a `Perthread*` for the calling thread.
    bool get_cache_dimensions(ImageHandle* file, Perthread* thread_info,
                              ImageSpec& spec, int subimage = 0,
                              int miplevel = 0);

    /// Copy into `thumbnail` any associated thumbnail associated with this
    /// image (for the first subimage by default, or as set by `subimage`).
    ///
    /// @param  filename
    ///             The name of the image, as a UTF-8 encoded ustring.
    /// @param  thumbnail
    ///             ImageBuf into which will be copied the thumbnail, if it
    ///             exists. If no thumbnail can be retrieved, `thumb` will
    ///             be reset to an uninitialized (empty) ImageBuf.
    /// @param  subimage
    ///             The subimage to query.
    /// @returns
    ///             `true` upon success, `false` upon failure failure (such
    ///             as being unable to find, open, or read the file, or if
    ///             it does not contain a thumbnail).
    bool get_thumbnail(ustring filename, ImageBuf& thumbnail, int subimage = 0);
    /// A more efficient variety of `get_thumbnail()` for cases where you
    /// can use an `ImageHandle*` to specify the image and optionally have a
    /// `Perthread*` for the calling thread.
    bool get_thumbnail(ImageHandle* file, Perthread* thread_info,
                       ImageBuf& thumbnail, int subimage = 0);
    /// @}

    /// @{
    /// @name   Getting Pixels

    /// For an image specified by name, retrieve the rectangle of pixels from
    /// the designated subimage and MIP level, storing the pixel values in the
    /// memory layout specified by `result`.  The pixel values will be
    /// converted to the data type specified by `format`. The rectangular
    /// region to be retrieved, specified by `roi`, includes `begin` but does
    /// not include `end` (much like STL begin/end usage). Requested pixels
    /// that are not part of the valid pixel data region of the image file
    /// will be filled with zero values.
    ///
    /// @param  filename
    ///             The name of the image, as a UTF-8 encoded ustring.
    /// @param  subimage/miplevel
    ///             The subimage and MIP level to retrieve pixels from.
    /// @param  roi
    ///             The range of pixels and channels to retrieve. The pixels
    ///             retrieved include the begin values but not the end values
    ///             (much like STL begin/end usage).
    /// @param  format
    ///             TypeDesc describing the data type of the values you want
    ///             to retrieve into `result`. The pixel values will be
    ///             converted to this type regardless of how they were
    ///             stored in the cache.
    /// @param  result
    ///             An `image_span` describing the memory layout where the
    ///             pixel values should be  stored, including bounds and
    ///             strides for each dimension.
    /// @param  cache_chbegin/cache_chend
    ///             These parameters can be used to tell the ImageCache to
    ///             read and cache a subset of channels (if not specified or
    ///             if they denote a non-positive range, all the channels of
    ///             the file will be stored in the cached tile).
    /// @returns    `true` upon success, or `false` upon failure.
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of read_scanlines that takes raw pointers.
    ///
    bool get_pixels(ustring filename, int subimage, int miplevel,
                    const ROI& roi, TypeDesc format,
                    const image_span<std::byte>& result, int cache_chbegin = 0,
                    int cache_chend = -1);
    /// A more efficient variety of `get_pixels()` for cases where you can use
    /// an `ImageHandle*` to specify the image and optionally have a
    /// `Perthread*` for the calling thread.
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of read_scanlines that takes raw pointers.
    bool get_pixels(ImageHandle* file, Perthread* thread_info, int subimage,
                    int miplevel, const ROI& roi, TypeDesc format,
                    const image_span<std::byte>& result, int cache_chbegin = 0,
                    int cache_chend = -1);

    /// A version of `get_pixels()` taking an `image_span<T>`, where the type
    /// of the underlying data is `T`.  This is a convenience wrapper around
    /// the `get_pixels()` that takes an `image_span<std::byte>`.
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of read_scanlines that takes raw pointers.
    template<typename T>
    bool get_pixels(ustring filename, int subimage, int miplevel,
                    const ROI& roi, const image_span<T>& result,
                    int cache_chbegin = 0, int cache_chend = -1)
    {
        static_assert(!std::is_const_v<T>,
                      "get_pixels() does not accept image_span<const T>");
        return get_pixels(filename, subimage, miplevel, roi,
                          TypeDescFromC<T>::value(),
                          as_image_span_writable_bytes(result), cache_chbegin,
                          cache_chend);
    }
    /// A more efficient variety of `get_pixels()` taking an `image_span<T>`,
    /// for cases where you can use an `ImageHandle*` to specify the image and
    /// optionally have a `Perthread*` for the calling thread.
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of read_scanlines that takes raw pointers.
    template<typename T>
    bool get_pixels(ImageHandle* file, Perthread* thread_info, int subimage,
                    int miplevel, const ROI& roi, const image_span<T>& result,
                    int cache_chbegin = 0, int cache_chend = -1)
    {
        static_assert(!std::is_const_v<T>,
                      "get_pixels() does not accept image_span<const T>");
        return get_pixels(file, thread_info, subimage, miplevel, roi,
                          TypeDescFromC<T>::value(),
                          as_image_span_writable_bytes(result), cache_chbegin,
                          cache_chend);
    }

    /// A version of `get_pixels()` taking a `span<T>`, which assumes
    /// contiguous strides in all dimensions. This is a convenience wrapper
    /// around the `get_pixels()` that takes an `image_span<T>`.
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of read_scanlines that takes raw pointers.
    template<typename T>
    bool get_pixels(ImageHandle* file, Perthread* thread_info, int subimage,
                    int miplevel, const ROI& roi, const span<T>& result,
                    int cache_chbegin = 0, int cache_chend = -1)
    {
        static_assert(!std::is_const_v<T>,
                      "get_pixels() does not accept span<const T>");
        auto ispan = image_span<T>(result.data(), roi.nchannels(), roi.width(),
                                   roi.height(), roi.depth());
        OIIO_DASSERT(result.size_bytes() == ispan.size_bytes()
                     && ispan.is_contiguous());
        return get_pixels(file, thread_info, subimage, miplevel, roi,
                          TypeDescFromC<T>::value(),
                          as_image_span_writable_bytes(result), cache_chbegin,
                          cache_chend);
    }
    /// A more efficient variety of `get_pixels()` taking a `span<T>`, for
    /// cases where you can use an `ImageHandle*` to specify the image and
    /// optionally have a `Perthread*` for the calling thread.
    ///
    /// Added in OIIO 3.1, this is the "safe" preferred alternative to
    /// the version of read_scanlines that takes raw pointers.
    template<typename T>
    bool get_pixels(ustring filename, int subimage, int miplevel,
                    const ROI& roi, const span<T>& result,
                    int cache_chbegin = 0, int cache_chend = -1)
    {
        static_assert(!std::is_const_v<T>,
                      "get_pixels() does not accept span<const T>");
        auto ispan = image_span<T>(result.data(), roi.nchannels(), roi.width(),
                                   roi.height(), roi.depth());
        OIIO_DASSERT(result.size_bytes() == ispan.size_bytes()
                     && ispan.is_contiguous());
        return get_pixels(filename, subimage, miplevel, roi,
                          TypeDescFromC<T>::value(),
                          as_image_span_writable_bytes(result), cache_chbegin,
                          cache_chend);
    }

    /// For an image specified by name, retrieve the rectangle of pixels
    /// from the designated subimage and MIP level, storing the pixel values
    /// beginning at the address specified by `result` and with the given
    /// strides.  The pixel values will be converted to the data type
    /// specified by `format`. The rectangular region to be retrieved
    /// includes `begin` but does not include `end` (much like STL begin/end
    /// usage). Requested pixels that are not part of the valid pixel data
    /// region of the image file will be filled with zero values.
    ///
    /// These pointer-based versions are considered "soft-deprecated" in
    /// OpenImageIO 3.1, will be marked/warned as deprecated in 3.2, and will
    /// be removed in 4.0.
    ///
    /// @param  filename
    ///             The name of the image, as a UTF-8 encoded ustring.
    /// @param  subimage/miplevel
    ///             The subimage and MIP level to retrieve pixels from.
    /// @param  xbegin/xend/ybegin/yend/zbegin/zend
    ///             The range of pixels to retrieve. The pixels retrieved
    ///             include the begin value but not the end value (much like
    ///             STL begin/end usage).
    /// @param  chbegin/chend
    ///             Channel range to retrieve. To retrieve all channels, use
    ///             `chbegin = 0`, `chend = nchannels`.
    /// @param  format
    ///             TypeDesc describing the data type of the values you want
    ///             to retrieve into `result`. The pixel values will be
    ///             converted to this type regardless of how they were
    ///             stored in the file.
    /// @param  result
    ///             Pointer to the memory where the pixel values should be
    ///             stored.  It is up to the caller to ensure that `result`
    ///             points to an area of memory big enough to accommodate
    ///             the requested rectangle (taking into consideration its
    ///             dimensions, number of channels, and data format).
    /// @param  xstride/ystride/zstride
    ///             The number of bytes between the beginning of successive
    ///             pixels, scanlines, and image planes, respectively. Any
    ///             stride values set to `AutoStride` will be assumed to
    ///             indicate a contiguous data layout in that dimension.
    /// @param  cache_chbegin/cache_chend These parameters can be used to
    ///             tell the ImageCache to read and cache a subset of
    ///             channels (if not specified or if they denote a
    ///             non-positive range, all the channels of the file will be
    ///             stored in the cached tile).
    ///
    /// @returns
    ///             `true` for success, `false` for failure.
    bool get_pixels(ustring filename, int subimage, int miplevel, int xbegin,
                    int xend, int ybegin, int yend, int zbegin, int zend,
                    int chbegin, int chend, TypeDesc format, void* result,
                    stride_t xstride = AutoStride,
                    stride_t ystride = AutoStride,
                    stride_t zstride = AutoStride, int cache_chbegin = 0,
                    int cache_chend = -1);
    /// A more efficient variety of `get_pixels()` for cases where you can
    /// use an `ImageHandle*` to specify the image and optionally have a
    /// `Perthread*` for the calling thread.
    ///
    /// These pointer-based versions are considered "soft-deprecated" in
    /// OpenImageIO 3.1, will be marked/warned as deprecated in 3.2, and will
    /// be removed in 4.0.
    bool get_pixels(ImageHandle* file, Perthread* thread_info, int subimage,
                    int miplevel, int xbegin, int xend, int ybegin, int yend,
                    int zbegin, int zend, int chbegin, int chend,
                    TypeDesc format, void* result,
                    stride_t xstride = AutoStride,
                    stride_t ystride = AutoStride,
                    stride_t zstride = AutoStride, int cache_chbegin = 0,
                    int cache_chend = -1);

    /// A simplified `get_pixels()` where all channels are retrieved,
    /// strides are assumed to be contiguous.
    ///
    /// These pointer-based versions are considered "soft-deprecated" in
    /// OpenImageIO 3.1, will be marked/warned as deprecated in 3.2, and will
    /// be removed in 4.0.
    bool get_pixels(ustring filename, int subimage, int miplevel, int xbegin,
                    int xend, int ybegin, int yend, int zbegin, int zend,
                    TypeDesc format, void* result);
    /// A more efficient variety of `get_pixels()` for cases where you can
    /// use an `ImageHandle*` to specify the image and optionally have a
    /// `Perthread*` for the calling thread.
    ///
    /// These pointer-based versions are considered "soft-deprecated" in
    /// OpenImageIO 3.1, will be marked/warned as deprecated in 3.2, and will
    /// be removed in 4.0.
    bool get_pixels(ImageHandle* file, Perthread* thread_info, int subimage,
                    int miplevel, int xbegin, int xend, int ybegin, int yend,
                    int zbegin, int zend, TypeDesc format, void* result);

    /// @}

    /// @{
    /// @name Controlling the cache
    ///

    /// Invalidate any loaded tiles or open file handles associated with the
    /// filename (UTF-8 encoded), so that any subsequent queries will be
    /// forced to re-open the file or re-load any tiles (even those that were
    /// previously loaded and would ordinarily be reused).  A client might do
    /// this if, for example, they are aware that an image being held in the
    /// cache has been updated on disk.  This is safe to do even if other
    /// procedures are currently holding reference-counted tile pointers from
    /// the named image, but those procedures will not get updated pixels
    /// until they release the tiles they are holding.
    ///
    /// If `force` is true, this invalidation will happen unconditionally; if
    /// false, the file will only be invalidated if it has been changed since
    /// it was first opened by the ImageCache.
    void invalidate(ustring filename, bool force = true);

    /// A more efficient variety of `invalidate()` for cases where you
    /// already have an `ImageHandle*` for the file you want to invalidate.
    void invalidate(ImageHandle* file, bool force = true);

    /// Invalidate all loaded tiles and close open file handles.  This is
    /// safe to do even if other procedures are currently holding
    /// reference-counted tile pointers from the named image, but those
    /// procedures will not get updated pixels (if the images change) until
    /// they release the tiles they are holding.
    ///
    /// If `force` is true, everything will be invalidated, no matter how
    /// wasteful it is, but if `force` is false, in actuality files will
    /// only be invalidated if their modification times have been changed
    /// since they were first opened.
    void invalidate_all(bool force = false);

    /// Close any open file handles associated with a named file (UTF-8
    /// encoded), but do not invalidate any image spec information or pixels
    /// associated with the files.  A client might do this in order to release
    /// OS file handle resources, or to make it safe for other processes to
    /// modify image files on disk.
    void close(ustring filename);

    /// `close()` all files known to the cache.
    void close_all();

    /// An opaque data type that allows us to have a pointer to a tile but
    /// without exposing any internals.
    using Tile = ImageCacheTile;

    /// Find the tile specified by an image filename (UTF-8 encoded), subimage
    /// & miplevel, the coordinates of a pixel, and optionally a channel
    /// range.   An opaque pointer to the tile will be returned, or `nullptr`
    /// if no such file (or tile within the file) exists or can be read.  The
    /// tile will not be purged from the cache until after `release_tile()` is
    /// called on the tile pointer the same number of times that `get_tile()`
    /// was called (reference counting). This is thread-safe! If `chend <
    /// chbegin`, it will retrieve a tile containing all channels in the file.
    Tile* get_tile(ustring filename, int subimage, int miplevel, int x, int y,
                   int z, int chbegin = 0, int chend = -1);
    /// A slightly more efficient variety of `get_tile()` for cases where
    /// you can use an `ImageHandle*` to specify the image and optionally
    /// have a `Perthread*` for the calling thread.
    ///
    /// @see `get_pixels()`
    Tile* get_tile(ImageHandle* file, Perthread* thread_info, int subimage,
                   int miplevel, int x, int y, int z, int chbegin = 0,
                   int chend = -1);

    /// After finishing with a tile, release_tile will allow it to
    /// once again be purged from the tile cache if required.
    void release_tile(Tile* tile) const;

    /// Retrieve the data type of the pixels stored in the tile, which may
    /// be different than the type of the pixels in the disk file.
    TypeDesc tile_format(const Tile* tile) const;

    /// Retrieve the ROI describing the pixels and channels stored in the
    /// tile.
    ROI tile_roi(const Tile* tile) const;

    /// For a tile retrieved by `get_tile()`, return a pointer to the pixel
    /// data itself, and also store in `format` the data type that the
    /// pixels are internally stored in (which may be different than the
    /// data type of the pixels in the disk file).   This method should only
    /// be called on a tile that has been requested by `get_tile()` but has
    /// not yet been released with `release_tile()`.
    const void* tile_pixels(Tile* tile, TypeDesc& format) const;

    /// The add_file() call causes a file to be opened or added to the
    /// cache. There is no reason to use this method unless you are
    /// supplying a custom creator, or configuration, or both.
    ///
    /// If creator is not NULL, it points to an ImageInput::Creator that
    /// will be used rather than the default ImageInput::create(), thus
    /// instead of reading from disk, creates and uses a custom ImageInput
    /// to generate the image. The 'creator' is a factory that creates the
    /// custom ImageInput and will be called like this:
    ///
    ///      std::unique_ptr<ImageInput> in (creator());
    ///
    /// Once created, the ImageCache owns the ImageInput and is responsible
    /// for destroying it when done. Custom ImageInputs allow "procedural"
    /// images, among other things.  Also, this is the method you use to set
    /// up a "writable" ImageCache images (perhaps with a type of ImageInput
    /// that's just a stub that does as little as possible).
    ///
    /// If `config` is not NULL, it points to an ImageSpec with configuration
    /// options/hints that will be passed to the underlying
    /// ImageInput::open() call. Thus, this can be used to ensure that the
    /// ImageCache opens a call with special configuration options.
    ///
    /// This call (including any custom creator or configuration hints) will
    /// have no effect if there's already an image by the same name in the
    /// cache. Custom creators or configurations only "work" the FIRST time
    /// a particular filename is referenced in the lifetime of the
    /// ImageCache. But if replace is true, any existing entry will be
    /// invalidated, closed and overwritten. So any subsequent access will
    /// see the new file. Existing texture handles will still be valid.
    bool add_file(ustring filename, ImageInput::Creator creator = nullptr,
                  const ImageSpec* config = nullptr, bool replace = false);

    /// Preemptively add a tile corresponding to the named image, at the
    /// given subimage, MIP level, and channel range.  The tile added is the
    /// one whose corner is (x,y,z), and buffer points to the pixels (in the
    /// given format, with supplied strides) which will be copied and
    /// inserted into the cache and made available for future lookups.
    /// If chend < chbegin, it will add a tile containing the full set of
    /// channels for the image. Note that if the 'copy' flag is false, the
    /// data is assumed to be in some kind of persistent storage and will
    /// not be copied, nor will its pixels take up additional memory in the
    /// cache.
    bool add_tile(ustring filename, int subimage, int miplevel, int x, int y,
                  int z, int chbegin, int chend, TypeDesc format,
                  const void* buffer, stride_t xstride = AutoStride,
                  stride_t ystride = AutoStride, stride_t zstride = AutoStride,
                  bool copy = true);

    /// Preemptively add a tile corresponding to the named image, at the given
    /// subimage, MIP level, and channel range.  The tile added is the one
    /// whose corner is (x,y,z), and buffer points to the pixels (in the given
    /// format) which will be copied and inserted into the cache and made
    /// available for future lookups. If chend < chbegin, it will add a tile
    /// containing the full set of channels for the image. Note that if the
    /// 'copy' flag is false, the data is assumed to be in some kind of
    /// persistent storage and will not be copied, nor will its pixels take up
    /// additional memory in the cache.
    bool add_tile(ustring filename, int subimage, int miplevel, int x, int y,
                  int z, int chbegin, int chend, TypeDesc format,
                  const image_span<const std::byte>& buffer, bool copy = true);

    /// A version of `add_tile()` taking an `image_span<T>`, where the type of
    /// the underlying data is `T`.  This is a convenience wrapper around the
    /// `add_tile()` that takes an `image_span<std::byte>`.
    template<typename T>
    bool add_tile(ustring filename, int subimage, int miplevel, int x, int y,
                  int z, int chbegin, int chend, const image_span<T>& buffer,
                  bool copy = true)
    {
        static_assert(!std::is_const_v<T>,
                      "add_tile() does not accept image_span<const T>");
        return add_tile(filename, subimage, miplevel, x, y, z, chbegin, chend,
                        TypeDescFromC<T>::value(), as_image_span_bytes(buffer),
                        copy);
    }

    /// @}

    /// @{
    /// @name Errors and statistics

    /// Is there a pending error message waiting to be retrieved?
    bool has_error() const;

    /// Return the text of all pending error messages issued against this
    /// ImageCache, and clear the pending error message unless `clear` is
    /// false. If no error message is pending, it will return an empty
    /// string.
    std::string geterror(bool clear = true) const;

    /// Returns a big string containing useful statistics about the
    /// ImageCache operations, suitable for saving to a file or outputting
    /// to the terminal. The `level` indicates the amount of detail in
    /// the statistics, with higher numbers (up to a maximum of 5) yielding
    /// more and more esoteric information.
    std::string getstats(int level = 1) const;

    /// Reset most statistics to be as they were with a fresh ImageCache.
    /// Caveat emptor: this does not flush the cache itelf, so the resulting
    /// statistics from the next set of texture requests will not match the
    /// number of tile reads, etc., that would have resulted from a new
    /// ImageCache.
    void reset_stats();

    /// @}

    ImageCache();
    ~ImageCache();

private:
    friend class TextureSystem;
    friend class TextureSystemImpl;

    // PIMPL idiom
    using Impl = ImageCacheImpl;
    static void impl_deleter(Impl*);
    std::unique_ptr<Impl, decltype(&impl_deleter)> m_impl;

    // User code should never directly construct or destruct an ImageCache.
    // Always use ImageCache::create() and ImageCache::destroy().
};


OIIO_NAMESPACE_END
