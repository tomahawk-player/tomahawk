# Packaging Tomahawk

General distribution agnostic packaging documentation for Linux.

## Stay up to date!

We will try to keep this document up to date, but we also let our packagers know if our dependencies get important updates - especially if they fix crashes or add new features - or if we add completely new ones. Last but not least, of course we want to let you know about new releases of Tomahawk!

If you want to be notified too subscribe to our [Google Group](https://groups.google.com/forum/#!forum/tomahawk-packagers).

## CMake build options

CMake build options are prefixed with ```-D``` and get their value passed after an ```=```.
You could for example specify the build type like this ```-DCMAKE_BUILD_TYPE=RelWithDebInfo```.
This document contains only options relevant to packaging, to have a complete overview you should check our CMake scripts yourself. Looking at it the other way round, this also means that you should read everything in here really carefully.


##### ```CMAKE_BUILD_TYPE```

Make sure to specify the build type as ```Release``` or even better ```RelWithDebInfo``` if your distribution supports debug packages. If you do not do this, your users might see asserts (that look like crashes to them) which are only really useful to developers.

##### ```CMAKE_INSTALL_LIBDIR``` (PATH)

You can specify the path where Tomahawk install the libs. This is very helpful to support multilib on linux machines. 

##### ```CMAKE_SKIP_RPATH``` (boolean) (default: OFF)

Build without using rpath prevents from overriding of the normal library search path, possibly interfering with local policy and causing problems for multilib, among other issues.

##### ```BUILD_RELEASE``` (boolean) (default: OFF)

If you're not using our tarballs you can turn this on to suppress putting Git revision hashes into the version string. This also disables building tools and tests by default (although you can do that manually) it's more future-proof to simply pass ```-DBUILD_RELEASE=ON``` because we might make use of it in later releases.

##### ```BUILD_TOOLS``` (boolean) (default: OFF, when BUILD_RELEASE=ON)

Tomahawk provides some tools that help highlight where crashes (of course we only crash in theory!) come from. To make them really useful, we need debug symbols to be available. If your distribution supports/allows it, you could put them into the -debug package.

##### ```BUILD_HATCHET``` (boolean) (default: ON)

Build the account plugin for Hatchet (http://hatchet.is). Requires [websocketpp](https://github.com/zaphoyd/websocketpp).

##### ```WITH_CRASHREPORTER``` (boolean) (default: ON)

The crash reporter is built by default if libcrashreporter-qt is available in ```thirdparty/libcrashreporter-qt/``` (for example via git submodule). Usually distributions don't allow packagers to upload debug symbols to the Tomahawk HQ so to give crash reports more meaning for us, that's why we have no standardised submit process in place yet. If you can do that in your distribution, please get in touch with us!

##### ```WITH_UPOWER``` (boolean) (default on Linux: ON)

Build with support for UPower events.

##### ```WITH_GNOMESHORTCUTHANDLER``` (boolean) (default on Linux: ON)

Build with shortcut handler for GNOME.

#### Runtime dependencies

##### QSql

If your distribution splits the QSqlite plugin for QSql into a separate package, make it a requirement of Tomahawk -  otherwise it might fail to start.

##### XMPP / jreen

Either jreen or (at least) Tomahawk's package should require the qca-ossl plugin, otherwise there will be no GTalk/Jabber support.

### Icon caches

In openSUSE there are macros for updating icon caches in KDE and GNOME (```%desktop_database_post[un]``` ```%icon_theme_cache_post[un]```) after [un]installation, check if your distribution offers the same.

### Firewall

Tomahawk offers P2P functionality, if your distribution offers a default firewall, it's nice to support a default profile for the standard Tomahawk P2P-port (50210). (cf. [openSUSE integration](https://build.opensuse.org/package/view_file/KDE:Extra/tomahawk/tomahawk.SuSEfirewall2?expand=1))
