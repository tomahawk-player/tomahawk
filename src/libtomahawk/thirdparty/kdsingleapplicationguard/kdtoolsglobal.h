#ifndef __KDTOOLS_KDTOOLSGLOBAL_H__
#define __KDTOOLS_KDTOOLSGLOBAL_H__

#include <QtCore/QtGlobal>

#define KDAB_DISABLE_COPY( x ) private: x( const x & ); x & operator=( const x & )

#ifdef KDTOOLS_SHARED
#  ifdef BUILD_SHARED_KDTOOLSCORE
#    define KDTOOLSCORE_EXPORT Q_DECL_EXPORT
#  else
#    define KDTOOLSCORE_EXPORT Q_DECL_IMPORT
#  endif
#  ifdef BUILD_SHARED_KDTOOLSGUI
#    define KDTOOLSGUI_EXPORT Q_DECL_EXPORT
#  else
#    define KDTOOLSGUI_EXPORT Q_DECL_IMPORT
#  endif
#  ifdef BUILD_SHARED_KDTOOLSXML
#    define KDTOOLSXML_EXPORT Q_DECL_EXPORT
#  else
#    define KDTOOLSXML_EXPORT Q_DECL_IMPORT
#  endif
#  ifdef BUILD_SHARED_KDUPDATER
#    define KDTOOLS_UPDATER_EXPORT    Q_DECL_EXPORT
#  else
#    define KDTOOLS_UPDATER_EXPORT    Q_DECL_IMPORT
#  endif
#else // KDTOOLS_SHARED
#  define KDTOOLSCORE_EXPORT
#  define KDTOOLSGUI_EXPORT
#  define KDTOOLSXML_EXPORT
#  define KDTOOLS_UPDATER_EXPORT
#endif // KDTOOLS_SHARED

#define MAKEINCLUDES_EXPORT

#define DOXYGEN_PROPERTY( x )
#ifdef DOXYGEN_RUN
# define KDAB_IMPLEMENT_SAFE_BOOL_OPERATOR( func ) operator unspecified_bool_type() const { return func; }
# define KDAB_USING_SAFE_BOOL_OPERATOR( Class ) operator unspecified_bool_type() const;
#else
# define KDAB_IMPLEMENT_SAFE_BOOL_OPERATOR( func )                      \
    private:                                                            \
        struct __safe_bool_dummy__ { void nonnull() {} };               \
    public:                                                             \
        typedef void ( __safe_bool_dummy__::*unspecified_bool_type )(); \
        operator unspecified_bool_type() const {                        \
            return ( func ) ? &__safe_bool_dummy__::nonnull : 0 ;       \
        }
#define KDAB_USING_SAFE_BOOL_OPERATOR( Class ) \
    using Class::operator Class::unspecified_bool_type;
#endif

#define KDTOOLS_MAKE_RELATION_OPERATORS( Class, linkage )             \
    linkage bool operator>( const Class & lhs, const Class & rhs ) {  \
        return operator<( rhs, lhs );                                 \
    }                                                                 \
    linkage bool operator!=( const Class & lhs, const Class & rhs ) { \
        return !operator==( lhs, rhs );                               \
    }                                                                 \
    linkage bool operator<=( const Class & lhs, const Class & rhs ) { \
        return !operator>( lhs, rhs );                                \
    }                                                                 \
    linkage bool operator>=( const Class & lhs, const Class & rhs ) { \
        return !operator<( lhs, rhs );                                \
    }

template <typename T>
inline T & __kdtools__dereference_for_methodcall( T & o ) {
    return o;
}

template <typename T>
inline T & __kdtools__dereference_for_methodcall( T * o ) {
    return *o;
}

#define KDAB_SET_OBJECT_NAME( x ) __kdtools__dereference_for_methodcall( x ).setObjectName( QLatin1String( #x ) )

KDTOOLSCORE_EXPORT bool _kdCheckQtVersion_impl( int major, int minor=0, int patchlevel=0 );
static inline bool kdCheckQtVersion( unsigned int major, unsigned int minor=0, unsigned int patchlevel=0 ) {
    return (major<<16|minor<<8|patchlevel) <= static_cast<unsigned int>(QT_VERSION)
	|| _kdCheckQtVersion_impl( major, minor, patchlevel );
}

#define KDTOOLS_DECLARE_PRIVATE_BASE( Class )                        \
protected:                                                           \
    class Private;                                                   \
    Private * d_func() { return _d; }                                \
    const Private * d_func() const { return _d; }                    \
    Class( Private * _d_, bool b ) : _d( _d_ ) { init(b); }          \
private:                                                             \
    void init(bool);                                                 \
private:                                                             \
    Private * _d

#define KDTOOLS_DECLARE_PRIVATE_DERIVED( Class, Base )                  \
protected:                                                              \
    class Private;                                                      \
    Private * d_func() {                                                \
        return reinterpret_cast<Private*>( Base::d_func() );            \
    }                                                                   \
    const Private * d_func() const {                                    \
        return reinterpret_cast<const Private*>( Base::d_func() );      \
    }                                                                   \
    Class( Private * _d_, bool b )                                      \
        : Base( reinterpret_cast<Base::Private*>(_d_), b ) { init(b); } \
private:                                                                \
    void init(bool)


#endif /* __KDTOOLS_KDTOOLSGLOBAL_H__ */

