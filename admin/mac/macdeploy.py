#!/usr/bin/python
#  This file is part of Tomahawk.
#  It was inspired in large part by the macdeploy script in Clementine.
#
#  Clementine is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Clementine is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Clementine.  If not, see <http://www.gnu.org/licenses/>.

import os
import re
import subprocess
import commands
import sys
import glob

TARGET_NAME="tomahawk"

FRAMEWORK_SEARCH_PATH=[
    '/Library/Frameworks',
    os.path.join(os.environ['HOME'], 'Library/Frameworks')
]

LIBRARY_SEARCH_PATH=['/usr/local/lib', '/usr/local/opt/vlc/lib', '/usr/local/Cellar/gettext/0.19.2/lib', '.']

VLC_PLUGINS=[
  'libattachment_plugin.dylib',
  #'libaccess_avio_plugin.dylib',
  #'libaccess_fake_plugin.dylib',
  'libftp_plugin.dylib',
  'libhttp_plugin.dylib',
  'libimem_plugin.dylib',
  #'libaccess_mmap_plugin.dylib',
  'libaccess_mms_plugin.dylib',
  'libaccess_realrtsp_plugin.dylib',
  'libtcp_plugin.dylib',
  'libudp_plugin.dylib',
  'libcdda_plugin.dylib',
  'libfilesystem_plugin.dylib',
  'libqtcapture_plugin.dylib',
  'librtp_plugin.dylib',
  'libzip_plugin.dylib',
  'libaccess_output_dummy_plugin.dylib',
  'libaccess_output_file_plugin.dylib',
  'libaccess_output_http_plugin.dylib',
  'libaccess_output_shout_plugin.dylib',
  'libaccess_output_udp_plugin.dylib',
  'liba52tofloat32_plugin.dylib',
  'liba52tospdif_plugin.dylib',
  'libaudio_format_plugin.dylib',
  'libaudiobargraph_a_plugin.dylib',
  'libchorus_flanger_plugin.dylib',
  #'libconverter_fixed_plugin.dylib',
  'libdolby_surround_decoder_plugin.dylib',
  'libdtstofloat32_plugin.dylib',
  'libdtstospdif_plugin.dylib',
  'libequalizer_plugin.dylib',
  'libheadphone_channel_mixer_plugin.dylib',
  'libmono_plugin.dylib',
  'libmpgatofixed32_plugin.dylib',
  'libnormvol_plugin.dylib',
  'libparam_eq_plugin.dylib',
  'libscaletempo_plugin.dylib',
  'libsimple_channel_mixer_plugin.dylib',
  'libspatializer_plugin.dylib',
  'libtrivial_channel_mixer_plugin.dylib',
  'libugly_resampler_plugin.dylib',
  'libfloat_mixer_plugin.dylib',
  #'libspdif_mixer_plugin.dylib',
  #'libtrivial_mixer_plugin.dylib',
  #'libaout_file_plugin.dylib',
  'libauhal_plugin.dylib',
  'liba52_plugin.dylib',
  'libadpcm_plugin.dylib',
  'libaes3_plugin.dylib',
  'libaraw_plugin.dylib',
  'libavcodec_plugin.dylib',
  'libcc_plugin.dylib',
  'libcdg_plugin.dylib',
  'libdts_plugin.dylib',
  'libfaad_plugin.dylib',
  #'libfake_plugin.dylib',
  'libflac_plugin.dylib',
  #'libfluidsynth_plugin.dylib',
  #'libinvmem_plugin.dylib',
  'liblpcm_plugin.dylib',
  'libmpeg_audio_plugin.dylib',
  'libpng_plugin.dylib',
  'librawvideo_plugin.dylib',
  'libspeex_plugin.dylib',
  'libspudec_plugin.dylib',
  'libtheora_plugin.dylib',
  'libtwolame_plugin.dylib',
  'libvorbis_plugin.dylib',
  #'libgestures_plugin.dylib',
  #'libhotkeys_plugin.dylib',
  #'libmotion_plugin.dylib',
  #'libnetsync_plugin.dylib',
  #'libsignals_plugin.dylib',
  'libaiff_plugin.dylib',
  'libasf_plugin.dylib',
  'libau_plugin.dylib',
  #'libavformat_plugin.dylib',
  'libavi_plugin.dylib',
  'libdemux_cdg_plugin.dylib',
  'libdemuxdump_plugin.dylib',
  'libdiracsys_plugin.dylib',
  'libes_plugin.dylib',
  'libflacsys_plugin.dylib',
  'liblive555_plugin.dylib',
  'libmkv_plugin.dylib',
  'libmod_plugin.dylib',
  'libmp4_plugin.dylib',
  'libmpc_plugin.dylib',
  'libmpgv_plugin.dylib',
  'libnsc_plugin.dylib',
  'libnsv_plugin.dylib',
  'libnuv_plugin.dylib',
  'libogg_plugin.dylib',
  'libplaylist_plugin.dylib',
  'libps_plugin.dylib',
  'libpva_plugin.dylib',
  'librawaud_plugin.dylib',
  'librawdv_plugin.dylib',
  'librawvid_plugin.dylib',
  'libreal_plugin.dylib',
  'libsmf_plugin.dylib',
  'libts_plugin.dylib',
  'libtta_plugin.dylib',
  'libty_plugin.dylib',
  'libvc1_plugin.dylib',
  'libvoc_plugin.dylib',
  'libwav_plugin.dylib',
  'libxa_plugin.dylib',
  'libfolder_plugin.dylib',
  'libtaglib_plugin.dylib',
  #'libaudioscrobbler_plugin.dylib',
  'libdummy_plugin.dylib',
  'libexport_plugin.dylib',
  #'libfreetype_plugin.dylib',
  #'libgnutls_plugin.dylib',
  'liblogger_plugin.dylib',
  'liblua_plugin.dylib',
  #'libosd_parser_plugin.dylib',
  #'libquartztext_plugin.dylib',
  #'libstats_plugin.dylib',
  'libvod_rtsp_plugin.dylib',
  'libxml_plugin.dylib',
  #'libxtag_plugin.dylib',
  'libi420_rgb_mmx_plugin.dylib',
  'libi420_yuy2_mmx_plugin.dylib',
  'libi422_yuy2_mmx_plugin.dylib',
  #'libmemcpymmx_plugin.dylib',
  #'libmemcpymmxext_plugin.dylib',
  'libmux_asf_plugin.dylib',
  'libmux_avi_plugin.dylib',
  'libmux_dummy_plugin.dylib',
  'libmux_mp4_plugin.dylib',
  'libmux_mpjpeg_plugin.dylib',
  'libmux_ogg_plugin.dylib',
  'libmux_ps_plugin.dylib',
  'libmux_ts_plugin.dylib',
  'libmux_wav_plugin.dylib',
  'libpacketizer_copy_plugin.dylib',
  'libpacketizer_dirac_plugin.dylib',
  'libpacketizer_flac_plugin.dylib',
  'libpacketizer_h264_plugin.dylib',
  'libpacketizer_mlp_plugin.dylib',
  'libpacketizer_mpeg4audio_plugin.dylib',
  'libpacketizer_mpeg4video_plugin.dylib',
  'libpacketizer_mpegvideo_plugin.dylib',
  'libpacketizer_vc1_plugin.dylib',
  'libi420_rgb_sse2_plugin.dylib',
  'libi420_yuy2_sse2_plugin.dylib',
  'libi422_yuy2_sse2_plugin.dylib',
  'libdecomp_plugin.dylib',
  #'libstream_filter_rar_plugin.dylib',
  'librecord_plugin.dylib',
  #'libvisual_plugin.dylib',
]

VLC_SEARCH_PATH=[
    '/usr/local/opt/vlc/lib/vlc/plugins/',
]

QT_PLUGINS = [
#    'crypto/libqca-ossl.dylib',
    'platforms/libqcocoa.dylib',
    'sqldrivers/libqsqlite.dylib',
    'imageformats/libqgif.dylib',
    'imageformats/libqico.dylib',
    'imageformats/libqjpeg.dylib',
    'imageformats/libqsvg.dylib',
    'imageformats/libqmng.dylib',
]

SNORE_PLUGINS = [
    'libsnore_backend_growl.so',
    'libsnore_backend_osxnotificationcenter.so',
]

TOMAHAWK_PLUGINS = [
  #'lib%s_account_xmpp.dylib' % TARGET_NAME,
  #'lib%s_account_google.so' % TARGET_NAME,
  'lib%s_account_zeroconf.so' % TARGET_NAME,
  'lib%s_account_hatchet.so' % TARGET_NAME,
  'lib%s_infoplugin_adium.so' % TARGET_NAME,
  'lib%s_infoplugin_charts.so' % TARGET_NAME,
#  'lib%s_infoplugin_discogs.so' % TARGET_NAME,
  'lib%s_infoplugin_echonest.so' % TARGET_NAME,
  'lib%s_infoplugin_hypem.so' % TARGET_NAME,
#  'lib%s_infoplugin_musicbrainz.so' % TARGET_NAME,
  'lib%s_infoplugin_musixmatch.so' % TARGET_NAME,
  'lib%s_infoplugin_newreleases.so' % TARGET_NAME,
#  'lib%s_infoplugin_rovi.so' % TARGET_NAME,
  'lib%s_infoplugin_snorenotify.so' % TARGET_NAME,
  'lib%s_infoplugin_spotify.so' % TARGET_NAME,
  #'lib%s_viewpage_dashboard.so' % TARGET_NAME,
#  'lib%s_viewpage_networkactivity.so' % TARGET_NAME,
  #'lib%s_viewpage_charts.so' % TARGET_NAME,
  #'lib%s_viewpage_newreleases.so' % TARGET_NAME,
  'lib%s_viewpage_whatsnew_0_8.so' % TARGET_NAME,
]

QT_PLUGINS_SEARCH_PATH=[
    '/usr/local/opt/qt5/plugins',
]

SNORE_PLUGINS_SEARCH_PATH=[
    '/usr/local/opt/snorenotify/lib/plugins/libsnore-qt5',
]

class Error(Exception):
  pass


class CouldNotFindQtPluginErrorFindFrameworkError(Error):
  pass


class InstallNameToolError(Error):
  pass

class CouldNotFindFrameworkError(Error):
  pass

class CouldNotFindQtPluginError(Error):
  pass


class CouldNotFindSnorePluginError(Error):
  pass


class CouldNotFindVLCPluginError(Error):
  pass


class CouldNotFindScriptPluginError(Error):
  pass


if len(sys.argv) < 2:
  print 'Usage: %s <bundle.app>' % sys.argv[0]

bundle_dir = sys.argv[1]
bundle_name = os.path.basename(bundle_dir).split('.')[0]

commands = []
framework_paths = []

binary_dir = os.path.join(bundle_dir, 'Contents', 'MacOS')
frameworks_dir = os.path.join(bundle_dir, 'Contents', 'Frameworks')
commands.append(['mkdir', '-p', frameworks_dir])
vlcplugins_dir = os.path.join(frameworks_dir, 'vlc', 'plugins')
commands.append(['mkdir', '-p', vlcplugins_dir])
snoreplugins_dir = os.path.join(binary_dir, 'libsnore')
commands.append(['mkdir', '-p', snoreplugins_dir])
resources_dir = os.path.join(bundle_dir, 'Contents', 'Resources')
commands.append(['mkdir', '-p', resources_dir])
plugins_dir = os.path.join(bundle_dir, 'Contents', 'qt-plugins')
binary = os.path.join(bundle_dir, 'Contents', 'MacOS', bundle_name)

fixed_libraries = []
fixed_frameworks = []

def GetBrokenLibraries(binary):
  #print "Checking libs for binary: %s" % binary
  output = subprocess.Popen(['otool', '-L', binary], stdout=subprocess.PIPE).communicate()[0]
  broken_libs = {
      'frameworks': [],
      'libs': []}
  for line in [x.split(' ')[0].lstrip() for x in output.split('\n')[1:]]:
    #print "Checking line: %s" % line
    if not line:  # skip empty lines
      continue
    if os.path.basename(binary) == os.path.basename(line):
      #print "mnope %s-%s" % (os.path.basename(binary), os.path.basename(line))
      continue
    if re.match(r'^\s*/System/', line):
      continue  # System framework
    elif re.match(r'^\s*/usr/lib/', line):
      #print "unix style system lib"
      continue  # unix style system library
    elif re.match(r'Breakpad', line):
      continue  # Manually added by cmake.
    elif re.match(r'^\s*@executable_path', line) or re.match(r'^\s*@loader_path', line) and not re.match(r'^\s*@loader_path/../lib', line):
      # Potentially already fixed library
      if '.framework' in line:
        relative_path = os.path.join(*line.split('/')[3:])
        if not os.path.exists(os.path.join(frameworks_dir, relative_path)):
          broken_libs['frameworks'].append(relative_path)
      else:
        relative_path = os.path.join(*line.split('/')[1:])
        #print "RELPATH %s %s" % (relative_path, os.path.join(binary_dir, relative_path))
        if not os.path.exists(os.path.join(binary_dir, relative_path)):
          broken_libs['libs'].append(relative_path)
    elif re.search(r'\w+\.framework', line):
      broken_libs['frameworks'].append(line)
    else:
      broken_libs['libs'].append(line)

  return broken_libs

def FindFramework(path):
  for search_path in FRAMEWORK_SEARCH_PATH:
    abs_path = os.path.join(search_path, path)
    if os.path.exists(abs_path):
      return abs_path

  # replace rpath with /Library/Frameworks for Sparkle
  abs_path = path.replace("@rpath/", "/Library/Frameworks/")
  if os.path.exists(abs_path):
    return abs_path

  raise CouldNotFindFrameworkError(path)

def FindLibrary(path):
  if os.path.exists(path):
    return path
  for search_path in LIBRARY_SEARCH_PATH:
    abs_path = os.path.join(search_path, path)
    if os.path.exists(abs_path):
      return abs_path
    else: # try harder---look for lib name in library folders
      newpath = os.path.join(search_path,os.path.basename(path))
      if os.path.exists(newpath):
        return newpath

  return ""
  #raise CouldNotFindFrameworkError(path)

def FixAllLibraries(broken_libs):
  for framework in broken_libs['frameworks']:
    FixFramework(framework)
  for lib in broken_libs['libs']:
    FixLibrary(lib)

def FixFramework(path):
  if path in fixed_libraries:
    return
  else:
    fixed_libraries.append(path)
  abs_path = FindFramework(path)
  broken_libs = GetBrokenLibraries(abs_path)
  FixAllLibraries(broken_libs)

  new_path = CopyFramework(abs_path)
  id = os.sep.join(new_path.split(os.sep)[3:])
  FixFrameworkId(new_path, id)
  for framework in broken_libs['frameworks']:
    FixFrameworkInstallPath(framework, new_path)
  for library in broken_libs['libs']:
    FixLibraryInstallPath(library, new_path)

def FixLibrary(path):
  if path in fixed_libraries or FindSystemLibrary(os.path.basename(path)) is not None:
    return
  else:
    fixed_libraries.append(path)
  abs_path = FindLibrary(path)
  if abs_path == "":
    print "Could not resolve %s, not fixing!" % path
    return
  broken_libs = GetBrokenLibraries(abs_path)
  FixAllLibraries(broken_libs)

  new_path = CopyLibrary(abs_path)
  FixLibraryId(new_path)
  for framework in broken_libs['frameworks']:
    FixFrameworkInstallPath(framework, new_path)
  for library in broken_libs['libs']:
    FixLibraryInstallPath(library, new_path)

def FixVLCPlugin(abs_path):
  broken_libs = GetBrokenLibraries(abs_path)
  FixAllLibraries(broken_libs)

  #print "Copying plugin....%s %s %s" % (plugins_dir, subdir, os.path.join(abs_path.split('/')[-2:]))
  new_path = os.path.join(vlcplugins_dir, os.path.basename(abs_path))
  args = ['mkdir', '-p', os.path.dirname(new_path)]
  commands.append(args)
  args = ['ditto', '--arch=i386', '--arch=x86_64', abs_path, new_path]
  commands.append(args)
  args = ['chmod', 'u+w', new_path]
  commands.append(args)
  for framework in broken_libs['frameworks']:
    FixFrameworkInstallPath(framework, new_path)
  for library in broken_libs['libs']:
    FixLibraryInstallPath(library, new_path)

def FixPlugin(abs_path, subdir):
  broken_libs = GetBrokenLibraries(abs_path)
  FixAllLibraries(broken_libs)

  new_path = CopyPlugin(abs_path, subdir)
  for framework in broken_libs['frameworks']:
    FixFrameworkInstallPath(framework, new_path)
  for library in broken_libs['libs']:
    FixLibraryInstallPath(library, new_path)

def FixBinary(path):
  broken_libs = GetBrokenLibraries(path)
  FixAllLibraries(broken_libs)
  for framework in broken_libs['frameworks']:
    FixFrameworkInstallPath(framework, path)
  for library in broken_libs['libs']:
    FixLibraryInstallPath(library, path)

def CopyLibrary(path):
  new_path = os.path.join(frameworks_dir, os.path.basename(path))
  args = ['ditto', '--arch=i386', '--arch=x86_64', path, new_path]
  commands.append(args)
  args = ['chmod', 'u+w', new_path]
  commands.append(args)
  return new_path

def CopyPlugin(path, subdir):
  new_path = os.path.join(plugins_dir, subdir, os.path.basename(path))
  args = ['mkdir', '-p', os.path.dirname(new_path)]
  commands.append(args)
  args = ['ditto', '--arch=i386', '--arch=x86_64', path, new_path]
  commands.append(args)
  args = ['chmod', 'u+w', new_path]
  commands.append(args)
  return new_path

def CopyFramework(path):
  parts = path.split(os.sep)
  for i, part in enumerate(parts):
    if re.match(r'\w+\.framework', part):
      full_path = os.path.join(frameworks_dir, *parts[i:-1])
      framework_name = part.split(".framework")[0]
      break

  if full_path in framework_paths:
    return os.path.join(full_path, parts[-1])

  framework_paths.append(full_path)

  args = ['mkdir', '-p', full_path]
  commands.append(args)
  args = ['ditto', '--arch=i386', '--arch=x86_64', path, full_path]
  commands.append(args)
  args = ['chmod', 'u+w', os.path.join(full_path, parts[-1])]
  commands.append(args)

  menu_nib = os.path.join(os.path.split(path)[0], 'Resources', 'qt_menu.nib')
  if os.path.exists(menu_nib):
    args = ['cp', '-rf', menu_nib, resources_dir]
    commands.append(args)

  # Fix framework structure for signing
  path_base_dir = os.path.join(os.path.split(path)[0], '..', '..')
  path_versions_dir = os.path.join(path_base_dir, 'Versions')

  if not os.path.exists(os.path.join(full_path, 'Versions', 'Current')):
    framework_base_dir = os.path.join(full_path, '..', '..')
    framework_versions_dir = os.path.join(framework_base_dir, 'Versions')

    versionParts = glob.glob(path_versions_dir+'/*')[0].split(os.sep)
    args = ['ln', '-s', versionParts[-1], framework_versions_dir+'/Current']
    commands.append(args)

    args = ['ln', '-s', 'Versions/Current/'+framework_name, framework_base_dir+'/'+framework_name]
    commands.append(args)

    args = ['ln', '-s', 'Versions/Current/Resources', framework_base_dir+'/Resources']
    commands.append(args)

  # Copy Contents/Info.plist to Resources/Info.plist if Resources/Info.plist does not exist
  # If Contents/Info.plist doesn't exist either, error out. If we actually see this, we can copy QtCore's Info.plist
  info_plist_in_resources = os.path.join(os.path.split(path)[0], '..', '..', 'Resources', 'Info.plist')
  if os.path.exists(info_plist_in_resources):
    info_plist_in_contents = os.path.join(os.path.split(path)[0], '..', '..', 'Resources', 'Info.plist')
    framework_resources_dir = os.path.join(framework_versions_dir, versionParts[-1], 'Resources')
    args = ['mkdir', '-p', framework_resources_dir]
    commands.append(args)
    if os.path.exists(info_plist_in_contents):
      args = ['cp', '-rf', info_plist_in_contents, framework_resources_dir]
      commands.append(args)
      args = ['chmod', '+rw', os.path.join(framework_resources_dir, 'Info.plist')]
      commands.append(args)
    else:
      print "%s: Framework does not contain an Info.plist file in Resources/ folder." % (path)
      sys.exit(-1)

  return os.path.join(full_path, parts[-1])

def FixId(path, library_name):
  id = '@executable_path/../Frameworks/%s' % library_name
  args = ['install_name_tool', '-id', id, path]
  commands.append(args)

def FixLibraryId(path):
  library_name = os.path.basename(path)
  FixId(path, library_name)

def FixFrameworkId(path, id):
  FixId(path, id)

def FixInstallPath(library_path, library, new_path):
  args = ['install_name_tool', '-change', library_path, new_path, library]
  commands.append(args)

def FindSystemLibrary(library_name):
  for path in ['/lib', '/usr/lib']:
    full_path = os.path.join(path, library_name)
    if os.path.exists(full_path):
      return full_path
  return None

def FixLibraryInstallPath(library_path, library):
  system_library = FindSystemLibrary(os.path.basename(library_path))
  if system_library is None:
    new_path = '@executable_path/../Frameworks/%s' % os.path.basename(library_path)
    FixInstallPath(library_path, library, new_path)
  else:
    FixInstallPath(library_path, library, system_library)

def FixFrameworkInstallPath(library_path, library):
  parts = library_path.split(os.sep)
  for i, part in enumerate(parts):
    if re.match(r'\w+\.framework', part):
      full_path = os.path.join(*parts[i:])
      break
  new_path = '@executable_path/../Frameworks/%s' % full_path
  FixInstallPath(library_path, library, new_path)

def FindQtPlugin(name):
  for path in QT_PLUGINS_SEARCH_PATH:
    if os.path.exists(path):
      if os.path.exists(os.path.join(path, name)):
        return os.path.join(path, name)
  raise CouldNotFindQtPluginError(name)

def FindSnorePlugin(name):
  for path in SNORE_PLUGINS_SEARCH_PATH:
    if os.path.exists(path):
      if os.path.exists(os.path.join(path, name)):
        return os.path.join(path, name)
  raise CouldNotFindSnorePluginError(name)

def FindVLCPlugin(name):
  for path in VLC_SEARCH_PATH:
    if os.path.exists(path):
      if os.path.exists(os.path.join(path, name)):
        return os.path.join(path, name)
  raise CouldNotFindVLCPluginError(name)

FixBinary(binary)

for plugin in VLC_PLUGINS:
  FixVLCPlugin(FindVLCPlugin(plugin))

for plugin in TOMAHAWK_PLUGINS:
  FixPlugin(plugin, '../MacOS')

for plugin in SNORE_PLUGINS:
  FixPlugin(FindSnorePlugin(plugin), '../lib/plugins/libsnore-qt5')

try:
  FixPlugin('%s_crash_reporter' % TARGET_NAME, '../MacOS')
except:
  print 'Failed to find %s_crash_reporter' % TARGET_NAME

for plugin in QT_PLUGINS:
  FixPlugin(FindQtPlugin(plugin), os.path.dirname(plugin))

if len(sys.argv) <= 2:
  print 'Would run %d commands:' % len(commands)
  for command in commands:
    print ' '.join(command)

  print 'OK?'
  raw_input()

for command in commands:
  p = subprocess.Popen(command)
  os.waitpid(p.pid, 0)
