///////////////////////////////////////////////////////////////////////////////
// Name:        wx/bmpbndl.h
// Purpose:     Declaration of wxBitmapBundle class.
// Author:      Vadim Zeitlin
// Created:     2021-09-22
// Copyright:   (c) 2021 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_BMPBNDL_H_
#define _WX_BMPBNDL_H_

#include "wx/bitmap.h"
#include "wx/object.h"
#include "wx/vector.h"

class wxBitmapBundleImpl;
class WXDLLIMPEXP_FWD_CORE wxWindow;

// It should be possible to implement SVG rasterizing without raw bitmap
// support using wxDC::DrawSpline(), but currently we don't do it and so
// FromSVG() is only available in the ports providing raw bitmap access.
#ifdef wxHAS_RAW_BITMAP
    #define wxHAS_SVG
#endif

// ----------------------------------------------------------------------------
// wxBitmapBundle provides 1 or more versions of a bitmap, all bundled together
// ----------------------------------------------------------------------------

// This class has value semantics and can be copied cheaply.

class WXDLLIMPEXP_CORE wxBitmapBundle
{
public:
    // Default ctor constructs an empty bundle which can't be used for
    // anything, but can be assigned something later.
    wxBitmapBundle();

    // This conversion ctor from a single bitmap does the same thing as
    // FromBitmap() and only exists for interoperability with the existing code
    // using wxBitmap.
    wxBitmapBundle(const wxBitmap& bitmap);

    // This is similar to above and also exists only for compatibility.
    wxBitmapBundle(const wxIcon& icon);

    // Another conversion ctor from a single image: this one is needed to allow
    // passing wxImage to the functions that used to take wxBitmap but now take
    // wxBitmapBundle.
    wxBitmapBundle(const wxImage& image);

    // Default copy ctor and assignment operator and dtor would be ok, but need
    // to be defined out of line, where wxBitmapBundleImpl is fully declared.

    wxBitmapBundle(const wxBitmapBundle& other);
    wxBitmapBundle& operator=(const wxBitmapBundle& other);

    ~wxBitmapBundle();


    // Create from the given collection of bitmaps (all of which must be valid,
    // but if the vector itself is empty, empty bundle is returned).
    static wxBitmapBundle FromBitmaps(const wxVector<wxBitmap>& bitmaps);
    static wxBitmapBundle FromBitmaps(const wxBitmap& bitmap1,
                                      const wxBitmap& bitmap2);

    // Create from a single bitmap (this is only useful for compatibility
    // with the existing code). Returns empty bundle if bitmap is invalid.
    static wxBitmapBundle FromBitmap(const wxBitmap& bitmap);
    static wxBitmapBundle FromImage(const wxImage& image);

    // It should be possible to implement SVG rasterizing without raw bitmap
    // support using wxDC::DrawSpline(), but currently we don't do it and so
    // FromSVG() is only available in the ports providing raw bitmap access.
#ifdef wxHAS_SVG
    // Create from the SVG data (data is supposed to be in UTF-8 encoding).
    // Notice that the data here is non-const because it can be temporarily
    // modified while parsing it.
    static wxBitmapBundle FromSVG(char* data, const wxSize& sizeDef);

    // This overload currently makes a copy of the data.
    static wxBitmapBundle FromSVG(const char* data, const wxSize& sizeDef);
#endif // wxHAS_SVG

    // Create from the resources: all existing versions of the bitmap of the
    // form name_2x or name@2x (and also using other factors) will be used.
    static wxBitmapBundle FromResources(const wxString& name);

    // Create from files: all existing versions of the bitmap of the
    // form filename_2x or name@2x (and also using other factors) will be used.
    static wxBitmapBundle FromFiles(const wxString& fullpathname);
    static wxBitmapBundle FromFiles(const wxString& path, const wxString& filename, const wxString& extension = wxASCII_STR("png"));

    // Create from existing implementation
    static wxBitmapBundle FromImpl(wxBitmapBundleImpl* impl);

    // Check if bitmap bundle is non-empty.
    bool IsOk() const { return m_impl; }

    // Get the size of the bitmap represented by this bundle when using the
    // default DPI, i.e. 100% scaling. Returns invalid size for empty bundle.
    wxSize GetDefaultSize() const;

    // Get preferred size, i.e. usually the closest size in which a bitmap is
    // available to the ideal size determined from the default size and the DPI
    // scaling, for the given window.
    wxSize GetPreferredSizeFor(const wxWindow* window) const;
    wxSize GetPreferredSizeAtScale(double scale) const;

    // Get bitmap of the specified size, creating a new bitmap from the closest
    // available size by rescaling it if necessary.
    //
    // If size == wxDefaultSize, GetDefaultSize() is used for it instead.
    wxBitmap GetBitmap(const wxSize& size) const;

    // Helper combining GetBitmap() and GetPreferredSizeFor(): returns the
    // bitmap of the size appropriate for the current DPI scaling of the given
    // window.
    wxBitmap GetBitmapFor(const wxWindow* window) const;

    // Access implementation
    wxBitmapBundleImpl* GetImpl() const { return m_impl.get(); }

private:
    typedef wxObjectDataPtr<wxBitmapBundleImpl> wxBitmapBundleImplPtr;

    // Private ctor used by static factory functions to create objects of this
    // class. It takes ownership of the pointer (which must be non-null).
    explicit wxBitmapBundle(wxBitmapBundleImpl* impl);

    wxBitmapBundleImplPtr m_impl;
};

// This macro can be used to create a bundle from resources on the platforms
// that support it and from name_png and name_2x_png on the other ones.
#ifdef wxHAS_IMAGE_RESOURCES
    #define wxBITMAP_BUNDLE_2(name) wxBitmapBundle::FromResources(#name)
#else
    #define wxBITMAP_BUNDLE_2(name)                                     \
        wxBitmapBundle::FromBitmaps(wxBITMAP_PNG_FROM_DATA(name),       \
                                    wxBITMAP_PNG_FROM_DATA(name##_2x))
#endif

// Inline functions implementation.

/* static */ inline
wxBitmapBundle wxBitmapBundle::FromBitmaps(const wxBitmap& bitmap1,
                                           const wxBitmap& bitmap2)
{
    wxVector<wxBitmap> bitmaps;
    if ( bitmap1.IsOk() )
        bitmaps.push_back(bitmap1);
    if ( bitmap2.IsOk() )
        bitmaps.push_back(bitmap2);
    return FromBitmaps(bitmaps);
}

/* static */ inline
wxBitmapBundle wxBitmapBundle::FromBitmap(const wxBitmap& bitmap)
{
    return wxBitmapBundle(bitmap);
}

/* static */ inline
wxBitmapBundle wxBitmapBundle::FromImage(const wxImage& image)
{
    if ( !image.IsOk() )
        return wxBitmapBundle();

    return FromBitmap(wxBitmap(image));
}

// ----------------------------------------------------------------------------
// wxBitmapBundleImpl is the base class for all wxBitmapBundle implementations
// ----------------------------------------------------------------------------

// This class inherits from wxRefCounter to make it possible to use it with
// wxObjectDataPtr in wxBitmapBundle.
//
// It doesn't need to be used directly, but may be inherited from in order to
// implement custom bitmap bundles.
class WXDLLIMPEXP_CORE wxBitmapBundleImpl : public wxRefCounter
{
protected:
    virtual ~wxBitmapBundleImpl();

public:
    // Return the size of the bitmaps represented by this bundle in the default
    // DPI (a.k.a. 100% resolution).
    //
    // Must always return a valid size.
    virtual wxSize GetDefaultSize() const = 0;

    // Return the preferred size that should be used at the given scale.
    //
    // Must always return a valid size.
    virtual wxSize GetPreferredSizeAtScale(double scale) const = 0;

    // Retrieve the bitmap of exactly the given size.
    //
    // Note that this function is non-const because it may generate the bitmap
    // on demand and cache it.
    virtual wxBitmap GetBitmap(const wxSize& size) = 0;
};

#endif // _WX_BMPBNDL_H_
