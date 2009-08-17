/*
 * Hemi - A Narwhal JavaScript engine
 * Copyright (c) 2009 Devin Torres
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <v8.h>
#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/iostreams/device/file.hpp>
#include <iostream>
#include <string>
#include <algorithm>
#include <iterator>
#include <cstdlib>

template<class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
  std::copy(v.begin(), v.end(), std::ostream_iterator<T>(std::cout, " "));
  return os;
}

namespace hemi {
  const std::string HEMI_VERSION = "0.1";

  namespace system {
    v8::Handle<v8::Value> print(const v8::Arguments& args) {
      const int len = args.Length();

      if (len > 0) {
        v8::HandleScope handle_scope;
        std::vector<std::string> v;

        for (int i = 0; i < len; ++i) {
          v.push_back(std::string(*v8::String::Utf8Value(args[i])));
        }

        std::cout << v << std::endl;
      }

      return v8::Undefined();
    }
  }

  namespace file {
    namespace fs = boost::filesystem;
    namespace io = boost::iostreams;

    v8::Handle<v8::Value> isFile(const v8::Arguments& args) {
      v8::HandleScope handle_scope;
      fs::path path(*v8::String::Utf8Value(args[0]), fs::native);
      return v8::Boolean::New(fs::exists(path) && fs::is_regular_file(path));
    }

    v8::Handle<v8::Value> read(const v8::Arguments& args) {
      v8::HandleScope handle_scope;
      std::string path(*v8::String::Utf8Value(args[0]));

      int len = fs::file_size(fs::path(path, fs::native));
      char* buffer = new char [len];

      io::file file(path, BOOST_IOS::in | BOOST_IOS::binary);

      if (!file.is_open()) return v8::Undefined();

      file.read(buffer, len);
      file.close();

      delete[] buffer;
      return v8::String::New(buffer, len);
    }
  }

  namespace shell {
    const bool prompt(std::string& input) {
      std::cout << ">> ";
      return std::getline(std::cin, input);
    }

    const void run() {
      std::cout << "Hemi version " << HEMI_VERSION << std::endl;

      std::string input;

      while (prompt(input)) {
        v8::Locker locker;
        v8::HandleScope handle_scope;

        v8::Handle<v8::String> source = v8::String::New(input.c_str());
        v8::Handle<v8::Script> script = v8::Script::Compile(source);

        v8::TryCatch trycatch;

        if (script.IsEmpty()) continue;

        v8::Handle<v8::Value> result = script->Run();

        if (result.IsEmpty()) {
          result = trycatch.Exception();
        }

        std::cout << *v8::String::Utf8Value(result) << std::endl;
      }
    }

    v8::Handle<v8::Value> quit(const v8::Arguments& args) {
      exit(args[0]->Int32Value());
      return v8::Undefined();
    }
  }
}

int main(int argc, char* argv[]) {
  try {
    namespace po = boost::program_options;

    po::options_description desc("Allowed options");
    desc.add_options()
      ("help", "produce help message")
      ("input-file", po::value< std::vector<std::string> >(), "input file")
    ;

    po::positional_options_description p;
    p.add("input-file", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 1;
    }

    if (vm.count("input-file")) {
      std::cout << "Input files are: " 
                << vm["input-file"].as< std::vector<std::string> >() << std::endl;
    }

    v8::Locker locker;
    v8::HandleScope handle_scope;
    v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();

    global->Set(v8::String::New("print"), v8::FunctionTemplate::New(hemi::system::print));
    global->Set(v8::String::New("isFile"), v8::FunctionTemplate::New(hemi::file::isFile));
    global->Set(v8::String::New("read"), v8::FunctionTemplate::New(hemi::file::read));
    global->Set(v8::String::New("quit"), v8::FunctionTemplate::New(hemi::shell::quit));

    v8::Persistent<v8::Context> context = v8::Context::New(NULL, global);
    v8::Context::Scope context_scope(context);

    hemi::shell::run();

    context.Dispose();
  } catch (std::exception& e) {
    std::cerr << "error: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Exception of unknown type!" << std::endl;
  }

  return 0;
}
