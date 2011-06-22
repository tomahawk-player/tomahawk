#ifndef _GROWL_H_
#define _GROWL_H_

#ifdef _WIN32
  #ifndef GROWL_STATIC
    #ifdef GROWL_DLL
      #define GROWL_EXPORT __declspec(dllexport)
    #else
      #define GROWL_EXPORT __declspec(dllimport)
    #endif
  #else
    #define GROWL_EXPORT
  #endif
#else
  #define GROWL_EXPORT
#endif //_WIN32

#ifdef __cplusplus
extern "C" {
#endif 


GROWL_EXPORT int growl( const char *const server,const char *const appname,const char *const notify,const char *const title, const char *const message ,
                                const char *const icon , const char *const password , const char *url );
GROWL_EXPORT int growl_tcp_notify( const char *const server,const char *const appname,const char *const notify,const char *const title, const char *const message ,
                                const char *const password, const char* const url, const char* const icon );
GROWL_EXPORT int growl_tcp_register( const char *const server , const char *const appname , const char **const notifications , const int notifications_count , const char *const password, const char *const icon );


GROWL_EXPORT int growl_udp( const char *const server,const char *const appname,const char *const notify,const char *const title, const char *const message ,
                                const char *const icon , const char *const password , const char *url );
GROWL_EXPORT int growl_udp_notify( const char *const server,const char *const appname,const char *const notify,const char *const title, const char *const message ,
                                const char *const password );
GROWL_EXPORT int growl_udp_register( const char *const server , const char *const appname , const char **const notifications , const int notifications_count , const char *const password  );


GROWL_EXPORT int growl_init(void);
GROWL_EXPORT void growl_shutdown(void);


#ifdef __cplusplus
}
#endif 


#endif /* _GROWL_H_ */
