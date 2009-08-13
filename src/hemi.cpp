#include <v8.h>
#include <boost/filesystem/operations.hpp>
#include <boost/iostreams/device/file.hpp>
#include <iostream>
#include <fstream>

namespace hemi {
  namespace fs = boost::filesystem;

  namespace system {
    v8::Handle<v8::Value> print(const v8::Arguments& args) {
      int len = args.Length();

      if (len > 0) {
        v8::HandleScope handle_scope;

        for (int i = 0; i < len - 1; ++i) {
          v8::String::Utf8Value str(args[i]);
          std::cout << *str << ' ';
        }

        v8::String::Utf8Value str(args[len-1]);
        std::cout << *str << std::endl;
      }

      return v8::Undefined();
    }
  }

  namespace file {
    v8::Handle<v8::Value> isFile(const v8::Arguments& args) {
      v8::HandleScope handle_scope;
      v8::String::Utf8Value str(args[0]);

      fs::path path(*str, fs::native);

      return v8::Boolean::New(fs::exists(path) && fs::is_regular_file(path));
    }

    v8::Handle<v8::Value> read(const v8::Arguments& args) {
      v8::HandleScope handle_scope;
      v8::String::Utf8Value str(args[0]);

      fs::path path(*str, fs::native);

      int len = fs::file_size(path);
      char* buffer = new char [len];

      std::ifstream file(*str, std::ios::in | std::ios::binary);
      file.read(buffer, len);
      file.close();

      delete[] buffer;
      return v8::String::New(buffer, len);
    }
  }

  v8::Handle<v8::Value> quit(const v8::Arguments& args) {
    exit(args[0]->Int32Value());
    return v8::Undefined();
  }
}

int main(int argc, char* argv[]) {
  // Create a stack-allocated handle scope.
  v8::HandleScope handle_scope;

  // Create a template for the global object.
  v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
  global->Set(v8::String::New("print"), v8::FunctionTemplate::New(hemi::system::print));
  global->Set(v8::String::New("isFile"), v8::FunctionTemplate::New(hemi::file::isFile));
  global->Set(v8::String::New("read"), v8::FunctionTemplate::New(hemi::file::read));
  global->Set(v8::String::New("quit"), v8::FunctionTemplate::New(hemi::quit));

  // Create a new context.
  v8::Persistent<v8::Context> context = v8::Context::New(NULL, global);
  
  // Enter the created context for compiling and
  // running the hello world script. 
  v8::Context::Scope context_scope(context);

  // Create a string containing the JavaScript source code.
  v8::Handle<v8::String> source = v8::String::New("(function(){ if (isFile('test')) { print(read('test')); }; quit(); })();");

  // Compile the source code.
  v8::Handle<v8::Script> script = v8::Script::Compile(source);

  // Run the script to get the result.
  v8::Handle<v8::Value> result = script->Run();

  // Dispose the persistent context.
  context.Dispose();

  // Convert the result to an ASCII string and print it.
  v8::String::AsciiValue ascii(result);
  printf("%s\n", *ascii);
  return 0;
}
