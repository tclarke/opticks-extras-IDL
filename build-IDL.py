#!/usr/local/bin/python
import sys
import subprocess
import os
import os.path
from os.path import join
import optparse
import traceback
import shutil
import zipfile

def execute_process(args, bufsize=0, executable=None, preexec_fn=None,
      close_fds=None, shell=False, cwd=None, env=None,
      universal_newlines=False, startupinfo=None, creationflags=0):
    if is_windows():
        stdin = subprocess.PIPE
        stdout = sys.stdout
        stderr = sys.stderr
    else:
        stdin = None
        stdout = None
        stderr = None

    process = subprocess.Popen(args, bufsize=bufsize, stdin=stdin,
          stdout=stdout, stderr=stderr, executable=executable,
          preexec_fn=preexec_fn, close_fds=close_fds, shell=shell,
          cwd=cwd, env=env, universal_newlines=universal_newlines,
          startupinfo=startupinfo, creationflags=creationflags)
    if is_windows():
        process.stdin.close()
    returncode = process.wait()
    return returncode

def is_windows():
    """Determine if this script is executing on the Windows operating system.
    @return: Return True if script is executed on Windows, False otherwise.
    @rtype: L{bool}

    """
    return sys.platform.startswith("win32")

class ScriptException(Exception):
    """Report error while running script"""

class Builder:
    def __init__(self, dependencies, opticks_code_dir, build_in_debug,
                 opticks_build_dir, verbosity):
        self.depend_path = dependencies
        self.opticks_code_dir = opticks_code_dir
        self.build_debug_mode = build_in_debug
        self.opticks_build_dir = opticks_build_dir
        self.verbosity = verbosity
        if self.build_debug_mode:
            self.mode = "debug"
        else:
            self.mode = "release"

    def build_executable(self, clean_build_first, build_opticks, concurrency):
        #No return code, throw exception or ScriptException
        if build_opticks == "none":
            return

        if self.verbosity > 1:
            print "Building IDL plug-ins..."
        buildenv = os.environ
        buildenv["OPTICKSDEPENDENCIES"] = self.depend_path
        buildenv["OPTICKS_CODE_DIR"] = self.opticks_code_dir

        if self.verbosity >= 1:
            print_env(buildenv)

        if clean_build_first:
            if self.verbosity > 1:
                print "Cleaning compilation..."
            self.compile_code(buildenv, True, concurrency)
            if self.verbosity > 1:
                print "Done cleaning compilation"

        self.compile_code(buildenv, False, concurrency)
        if self.verbosity > 1:
            print "Done building IDL plug-ins"

    def prep_to_run_helper(self, plugin_suffixes):
        if self.verbosity > 1:
            print "Copying Opticks plug-ins into IDL workspace..."
        extension_plugin_path = join(self.get_binaries_dir(), "PlugIns")
        if not os.path.exists(extension_plugin_path):
            os.makedirs(extension_plugin_path)
        opticks_plugin_path = os.path.abspath(self.get_comet_plugin_dir())
        copy_files_in_dir(opticks_plugin_path, extension_plugin_path, plugin_suffixes)
        if self.verbosity > 1:
            print "Done copying Opticks plug-ins"

        extension_bin_path = join(self.get_binaries_dir(), "Bin")
        if not os.path.exists(extension_bin_path):
            os.makedirs(extension_bin_path)
        extension_dep_file = join(extension_bin_path, "IDL.dep")
        if not os.path.exists(extension_dep_file):
            if self.verbosity > 1:
                print "Creating IDL.dep file..."
            extension_dep = open(extension_dep_file, "w")
            extension_dep.write("!depV1 { deployment: { "\
                "AppHomePath: $E(OPTICKS_HOME), "\
                "AdditionalDefaultPath: ../../../../Release/DefaultSettings, "\
                "UserConfigPath: ../../ApplicationUserSettings, "\
                "PlugInPath: ../PlugIns } } ")
            extension_dep.close()
            if self.verbosity > 1:
                print "Done creating IDL.dep file"

        app_setting_dir = join("Code", "Build", "ApplicationUserSettings")
        if not os.path.exists(app_setting_dir):
            if self.verbosity > 1:
                print "Creating ApplicationUserSettings folder at %s..." % \
                    (app_setting_dir)
            os.makedirs(app_setting_dir)
            if self.verbosity > 1:
                print "Done creating ApplicationUserSettings folder"

    def build_doxygen(self, build, artifacts_dir):
        if self.verbosity > 1:
            print "Generating HTML..."
        doc_path = os.path.abspath(join("Code", "Build", "DoxygenOutput"))
        if os.path.exists(doc_path):
            #delete any already generated documentation
            shutil.rmtree(doc_path, True)
        os.makedirs(doc_path)
        doxygen_cmd = self.get_doxygen_path()
        config_dir = os.path.abspath(join("Code", "ApiDocs"))
        args = [doxygen_cmd, join(config_dir, "application.dox")]
        env = os.environ
        env["SOURCE"] = os.path.abspath("Code")
        env["OUTPUT_DIR"] = doc_path
        env["CONFIG_DIR"] = config_dir
        graphviz_dir = os.path.abspath(join(self.depend_path,
            "graphviz", "app"))
        env["DOT_DIR"] = join(graphviz_dir, "bin")
        self.other_doxygen_prep(build, env)
        retcode = execute_process(args, env=env)
        if retcode != 0:
            raise ScriptException("Unable to run doxygen generation script")
        if self.verbosity > 1:
            print "Done generating HTML"
        if artifacts_dir is not None:
            if self.verbosity > 1:
                print "Compressing Doxygen because --artifact-dir "\
                    "was provided."
            html_path = join(doc_path, "html")
            zip_name = "doxygen.zip"
            zip_path = os.path.abspath(join(artifacts_dir, zip_name))
            the_zip = zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED)
            for cur_dir, dirs, files in os.walk(html_path):
                arc_dir = cur_dir[len(html_path):]
                for the_file in files:
                    the_zip.write(join(cur_dir, the_file),
                        join(arc_dir,the_file))
            the_zip.close()
            if self.verbosity > 1:
                print "Done compressing Doxygen"

    def __read_version_h(self):
        version_path = join("Code", "Include", "IDLVersion.h")
        version_info = open(version_path, "rt").readlines()
        rdata = {}
        for vline in version_info:
            fields = vline.strip().split()
            if len(fields) >=3 and fields[0] == "#define":
                rdata[fields[1]] = " ".join(fields[2:])
        return rdata

    def build_installer(self):
        os.environ['PATH'] += os.pathsep + join(self.depend_path, "raptor", "Bin", self.platform)
        try:
            import raptor
        except:
            print os.environ['PATH']
            raise ScriptException("Unable to locate raptor. Make sure the raptor dependency is installed.")
        PF_AEBL = "urn:2008:03:aebl-syntax-ns#"
        PF_OPTICKS = "urn:2008:03:opticks-aebl-extension-ns#"

        if self.verbosity > 1:
            print "Loading metadata template..."
        parser = raptor.RaptorParser()
        parser.set_feature(raptor.RAPTOR_FEATURE_NO_NET)
        installer_path = os.path.abspath("Installer")
        parser.parse_file(join(installer_path, "install.n3"))
        metadata = parser.statements()
        parser.cleanup()

        manifest = metadata["urn:aebl:install-manifest"]
        version_info = self.__read_version_h()
        manifest[PF_AEBL + "version"] = [version_info["IDL_VERSION_NUMBER"]]
        manifest[PF_AEBL + "name"] = [version_info["IDL_NAME"]]
        manifest[PF_AEBL + "description"] = [version_info["IDL_NAME_LONG"]]
        manifest[PF_AEBL + "targetPlatform"] = [self.aeb_platform]

        out_path = os.path.abspath(join("Code", "Build", "Installer"))
        if os.path.exists(out_path):
            shutil.rmtree(out_path, True)
        os.makedirs(out_path)
        if self.verbosity > 1:
            print "Saving updated metadata to AEB..."
        serializer = raptor.RaptorSerializer("rdfxml-abbrev")
        serializer.statements(metadata)
        install_rdf = serializer.serialize_to_string()
        serializer.cleanup()

        if self.verbosity > 1:
            print "Building installation tree..."
        zfile = zipfile.ZipFile(join(out_path, "IDL.aeb"), "w", zipfile.ZIP_DEFLATED)

        # platform independent items
        zfile.writestr("install.rdf", install_rdf)
        extension_settings_dir = join(os.path.abspath("Release"), "DefaultSettings")
        copy_files_in_dir_to_zip(extension_settings_dir, join("content", "DefaultSettings"), zfile, [".cfg"], ["_svn", ".svn"])
        copy_file_to_zip(os.path.abspath("Release"), "license", "lgpl-2.1.txt", zfile)
        doc_path = os.path.abspath(join("Code", "Build", "DoxygenOutput"))
        copy_files_in_dir_to_zip(doc_path, join("content", "Help", "IDL"), zfile)

        # platform dependent items
        self.build_installer_platform(zfile)

        zfile.close()

class WindowsBuilder(Builder):
    def __init__(self, dependencies, opticks_code_dir, build_in_debug,
                 opticks_build_dir, visualstudio, verbosity):
        Builder.__init__(self, dependencies, opticks_code_dir, build_in_debug,
            opticks_build_dir, verbosity)
        self.vs_path = visualstudio

    def compile_code(self, env, clean, concurrency):
        solution_file = os.path.abspath("Code/IDL.sln")
        self.build_in_visual_studio(solution_file,
            self.build_debug_mode, self.is_64_bit, concurrency,
            self.vs_path, env, clean)

    def get_comet_plugin_dir(self):
        return os.path.abspath(join(self.opticks_build_dir,
            "Binaries-%s-%s" % (self.platform, self.mode), "PlugIns"))

    def get_binaries_dir(self):
        if self.platform == "Win32":
            arch = "32"
        elif self.platform == "x64":
            arch = "64"
        build_dir = os.path.join(os.path.abspath("Code"), "Build")
        return os.path.abspath(join(build_dir,
            "Binaries-%s-%s" % (self.platform, self.mode)))
    
    def get_doxygen_path(self):
        return join(self.depend_path, "doxygen", "bin", "doxygen.exe")

    def other_doxygen_prep(self, build, env):
        if build != "all":
            return
        if self.verbosity > 1:
            print "Enabling CHM generation"
        hhc_path = os.path.abspath(join(self.ms_help_compiler, "hhc.exe"))
        if not(os.path.exists(hhc_path)):
            raise ScriptException("MS Help Compiler path of %s is "\
                "invalid, see --ms-help-compiler" %
                (self.ms_help_compiler))
        env["GENERATE_CHM"] = "YES"
        env["MICROSOFT_HELP_COMPILER"] = hhc_path
        chm_file = "IDL.chm"
        env["CHM_NAME"] = chm_file

    def prep_to_run(self):
        self.prep_to_run_helper([".dll", ".exe"])

    def build_in_visual_studio(self, solutionfile, debug,
                               build_64_bit, concurrency, vspath,
                               environ, clean):
        if debug and not build_64_bit:
            mode = "Debug|Win32"
        if not debug and not build_64_bit:
            mode = "Release|Win32"
        if debug and build_64_bit:
            mode = "Debug|x64"
        if not debug and build_64_bit:
            mode = "Release|x64"

        msdev_exec = join(vspath, "vc", "vcpackages", "vcbuild.exe")
        arguments = [msdev_exec, solutionfile]
        if clean:
            arguments.append("/clean")
        arguments.append("/error:[ERROR]")
        arguments.append("/warning:[WARNING]")
        arguments.append("/M%s" % (concurrency))
        arguments.append(mode)
        ret_code = execute_process(arguments,
                                 env=environ)
        if ret_code != 0:
            raise ScriptException("Visual Studio did not compile project")

    def build_installer_platform(self, zfile):
        extension_plugin_path = join(self.get_binaries_dir(), "PlugIns")
        target_plugin_path = join("platform", self.aeb_platform, "PlugIns")
        copy_file_to_zip(extension_plugin_path, target_plugin_path, "IdlInterpreter.dll", zfile)
        copy_file_to_zip(extension_plugin_path, target_plugin_path, "IdlStart.dll", zfile)

class Windows32bitBuilder(WindowsBuilder):
    def __init__(self, dependencies, opticks_code_dir, build_in_debug,
                 opticks_build_dir, visualstudio, verbosity):
        WindowsBuilder.__init__(self, dependencies, opticks_code_dir, build_in_debug,
            opticks_build_dir, visualstudio, verbosity)
        self.is_64_bit = False
        self.platform = "Win32"
        if build_in_debug:
            self.aeb_platform = "win32-x86-msvc8.1-debug"
        else:
            self.aeb_platform = "win32-x86-msvc8.1-release"

class Windows64bitBuilder(WindowsBuilder):
    def __init__(self, dependencies, opticks_code_dir, build_in_debug,
                 opticks_build_dir, visualstudio, verbosity):
        WindowsBuilder.__init__(self, dependencies, opticks_code_dir, build_in_debug,
            opticks_build_dir, visualstudio, verbosity)
        self.is_64_bit = True
        self.platform = "x64"
        if build_in_debug:
            self.aeb_platform = "win64-x86-msvc8.1-debug"
        else:
            self.aeb_platform = "win64-x86-msvc8.1-release"

class SolarisBuilder(Builder):
    def __init__(self, dependencies, opticks_code_dir, build_in_debug,
                 opticks_build_dir, verbosity):
        Builder.__init__(self, dependencies, opticks_code_dir, build_in_debug,
            opticks_build_dir, verbosity)
        self.platform = "solaris-sparc"
        if build_in_debug:
            self.aeb_platform = "solaris-sparc-studio12-debug"
        else:
            self.aeb_platform = "solaris-sparc-studio12-release"

    def get_doxygen_path(self):
        return join(self.depend_path, "doxygen", "bin", "doxygen")

    def other_doxygen_prep(self, build, env):
        graphviz_dir = os.path.abspath(join(self.depend_path,
            "graphviz", "app"))
        env["GVBINDIR"] = join(graphviz_dir, "lib", "graphviz")
        new_value = join(graphviz_dir, "lib")
        if env.has_key("LD_LIBRARY_PATH_32"):
            new_value = new_value + ":" + env["LD_LIBRARY_PATH_32"]
        env["LD_LIBRARY_PATH_32"] = new_value

    def compile_code(self, env, clean, concurrency):
        #Build extension plugins
        self.run_scons(os.path.abspath("."), self.build_debug_mode,
            concurrency, env, clean, ["all"])

    def run_scons(self, path, debug, concurrency, environ,
                  clean, extra_args=None):
        scons_exec = "scons"
        arguments = [scons_exec, "--directory=Code", "--no-cache"]
        arguments.append("-j%s" % (concurrency))
        if clean:
            arguments.append("-c")
        if not debug:
            arguments.append("RELEASE=yes")
        if extra_args:
            arguments.extend(extra_args)
        ret_code = execute_process(arguments,
                                 env=environ,
                                 cwd=path)
        if ret_code != 0:
            raise ScriptException("Scons did not compile project")

    def get_comet_plugin_dir(self):
        return os.path.abspath(join(self.opticks_build_dir,
            "Binaries-solaris-sparc-%s" % (self.mode), "PlugIns"))

    def get_binaries_dir(self):
        return os.path.abspath(join("Code", "Build",
            "Binaries-solaris-sparc-%s" % (self.mode)))

    def prep_to_run(self):
        self.prep_to_run_helper([".so"])

    def build_installer_platform(self, zfile):
        extension_plugin_path = join(self.get_binaries_dir(), "PlugIns")
        platform = self.aeb_platform
        if self.build_debug_mode:
            platform += "-debug"
        else:
            platform += "-release"
        target_plugin_path = join("platform", self.aeb_platform, "PlugIns")
        copy_file_to_zip(extension_plugin_path, target_plugin_path, "IdlInterpreter.so", zfile)
        copy_file_to_zip(extension_plugin_path, target_plugin_path, "IdlStart.so", zfile)


def print_env(environ):
    print "Environment is currently set to"
    for key in environ.iterkeys():
        print key, "=", environ[key]

def copy_files_in_dir(src_dir, dst_dir, suffixes_to_match=[]):
    if not os.path.exists(dst_dir):
        os.makedirs(dst_dir)
    for entry in os.listdir(src_dir):
        src_path = join(src_dir, entry)
        if os.path.isfile(src_path):
            matches = False
            if suffixes_to_match is None or len(suffixes_to_match) == 0:
                matches = True
            else:
                for suffix in suffixes_to_match:
                    if entry.endswith(suffix):
                        matches = True
            if matches:
                copy_file(src_dir, dst_dir, entry)
        elif os.path.isdir(src_path):
            dst_path = join(dst_dir, entry)
            copy_files_in_dir(src_path, dst_path, suffixes_to_match)

def copy_file(src_dir, dst_dir, filename):
    dst_file = join(dst_dir, filename)
    if os.path.exists(dst_file):
        os.remove(dst_file)
    shutil.copy2(join(src_dir, filename),
                 dst_file)

def copy_files_in_dir_to_zip(src_dir, dst_dir, zfile, suffixes_to_match=None, entries_to_skip=None):
    for entry in os.listdir(src_dir):
        if entries_to_skip is not None and entry in entries_to_skip:
            continue
        src_path = join(src_dir, entry)
        if os.path.isfile(src_path):
            matches = False
            if suffixes_to_match is None or len(suffixes_to_match) == 0:
                matches = True
            else:
                for suffix in suffixes_to_match:
                    if entry.endswith(suffix):
                        matches = True
            if matches:
                copy_file_to_zip(src_dir, dst_dir, entry, zfile)
        elif os.path.isdir(src_path):
            dst_path = join(dst_dir, entry)
            copy_files_in_dir_to_zip(src_path, dst_path, zfile, suffixes_to_match, entries_to_skip)

def copy_file_to_zip(src_dir, dst_dir, filename, zfile):
    dst_file = join(dst_dir, filename)
    zfile.writestr(dst_file, open(join(src_dir, filename), "rb").read())

def main(args):
    #chdir to the directory where the script resides
    os.chdir(os.path.abspath(os.path.dirname(sys.argv[0])))

    options = optparse.OptionParser()
    options.add_option("-d", "--dependencies",
        dest="dependencies", action="store", type="string")
    options.add_option("--opticks-code-dir",
        dest="opticks_code_dir", action="store", type="string")
    if is_windows():
        vs_path = "C:\\Program Files (x86)\\Microsoft Visual Studio 8"
        if not os.path.exists(vs_path):
            vs_path = "C:\\Program Files\\Microsoft Visual Studio 8"
        options.add_option("--visualstudio", dest="visualstudio",
            action="store", type="string")
        options.add_option("--arch", dest="arch", action="store",
            type="choice", choices=["32","64"])
        options.set_defaults(visualstudio=vs_path, arch="64")
    options.add_option("-m", "--mode", dest="mode",
        action="store", type="choice", choices=["debug", "release"])
    options.add_option("--clean", dest="clean", action="store_true")
    options.add_option("--build-extension", dest="build_extension",
        action="store", type="choice", choices=["all","none"])
    options.add_option("--prep", dest="prep", action="store_true")
    options.add_option("--build-installer", dest="build_installer", action="store_true")
    options.add_option("--concurrency", dest="concurrency", action="store")
    options.add_option("--build-doxygen", dest="build_doxygen")
    options.add_option("-q", "--quiet", help="Print fewer messages",
        action="store_const", dest="verbosity", const=0)
    options.add_option("-v", "--verbose", help="Print more messages",
        action="store_const", dest="verbosity", const=2)
    options.set_defaults(mode="release", clean=False,
        build_extension="none", build_doxygen="none",
        build_installer=False, prep=False, concurrency=1, verbosity=1)
    options = options.parse_args(args[1:])[0]

    builder = None
    try:
        opticks_depends = os.environ.get("OPTICKSDEPENDENCIES", None)
        if options.dependencies:
            #allow the -d command-line option to override
            #environment variable
            opticks_depends = options.dependencies
        if not opticks_depends:
            #didn't use -d command-line option, nor an environment variable
            #so consider that an error
            raise ScriptException("Dependencies argument must be provided")
        if not os.path.exists(opticks_depends):
            raise ScriptException("Dependencies path is invalid")

        opticks_code_dir = os.environ.get("OPTICKS_CODE_DIR", None)
        if options.opticks_code_dir:
            opticks_code_dir = options.opticks_code_dir
        if not opticks_code_dir:
            raise ScriptException("Opticks code dir argument must be provided")
        if not os.path.exists(opticks_code_dir):
            raise ScriptException("Opticks code dir is invalid")

        opticks_build_dir = join(opticks_code_dir, "Build")
        if not opticks_build_dir:
            raise ScriptException("Opticks build directory argument "\
                "must be provided")
        if not os.path.exists(opticks_build_dir):
            raise ScriptException("Opticks build directory path is invalid")

        if options.mode == "debug":
            build_in_debug = True
        else:
            build_in_debug = False

        if not is_windows():
            builder = SolarisBuilder(opticks_depends, opticks_code_dir, build_in_debug,
                opticks_build_dir, options.verbosity)
        else:
            if not os.path.exists(options.visualstudio):
                raise ScriptException("Visual Studio path is invalid")

            if options.arch == "32":
                builder = Windows32bitBuilder(opticks_depends, opticks_code_dir,
                    build_in_debug, opticks_build_dir,
                    options.visualstudio, options.verbosity)
            if options.arch == "64":
                builder = Windows64bitBuilder(opticks_depends, opticks_code_dir,
                    build_in_debug, opticks_build_dir,
                    options.visualstudio, options.verbosity)

        builder.build_executable(options.clean, options.build_extension,
            options.concurrency)

        if options.build_doxygen != "none":
           if options.verbosity > 1:
              print "Building doxygen..."
           builder.build_doxygen(options.build_doxygen, None)
           if options.verbosity > 1:
              print "Done building doxygen"

        if options.prep:
            if options.verbosity > 1:
                print "Prepping to run..."
            builder.prep_to_run()
            if options.verbosity > 1:
                print "Done prepping to run"

        if options.build_installer:
            if options.verbosity > 1:
                print "Building AEB installation extension..."
            builder.build_installer()
            if options.verbosity > 1:
                print "Done building installer"

    except Exception, e:
        print "--------------------------"
        traceback.print_exc()
        print "--------------------------"
        return 2000

    return 0

if __name__ == "__main__":
    sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)
    retcode = main(sys.argv)
    print "Return code is", retcode
    sys.exit(retcode)
