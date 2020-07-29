TEMPLATE = app
LIBS += -L$$OUT_PWD/../Common -lCommon
LIBS += -luser32 -lole32 -lgdi32 -lcomctl32 -ld2d1 -ldwrite -lshlwapi -lstructuredquery
INCLUDEPATH += ../Common/include

SOURCES += AsyncLoaderHelper.cpp\
    Browser.cpp\
    BrowserApplication.cpp\
    CarouselAnimation.cpp\
    CarouselPane.cpp\
    CarouselThumbnail.cpp\
    CarouselThumbnailAnimation.cpp\
    Flickr.wsdl.c\
    FlickrUploader.cpp\
    FlyerAnimation.cpp\
    ImageThumbnailControl.cpp\
    LineAnimation.cpp\
    MediaPane.cpp\
    MediaPaneAnimation.cpp\
    MoverAnimation.cpp\
    OrbitAnimation.cpp\
    PanAnimation.cpp\
    ShareDialog.cpp\
    SlideAnimation.cpp\
    ThumbnailImpl.cpp\
    ThumbnailLayoutManager.cpp\
    AsyncLoader/AsyncLoader.cpp\
    AsyncLoader/AsyncLoaderItemCache.cpp\
    AsyncLoader/AsyncLoaderLayoutManager.cpp\
    AsyncLoader/AsyncLoaderMemoryManager.cpp\
    AsyncLoader/LoadableItemList.cpp\
    AsyncLoader/MemorySizeConverter.cpp

HEADERS += Animation.h\
    AsyncLoaderHelper.h\
    AsyncLoaderHelperInterface.h\
    Browser.h\
    CarouselAnimation.h\
    CarouselPane.h\
    Flickr.wsdl.h\
    FlickrUploader.h\
    FlyerAnimation.h\
    ImageThumbnailControl.h\
    LineAnimation.h\
    resource.h


