/*
 * Copyright (c) 2022, May Neelon <mayflowerc10@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/StringView.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/File.h>
#include <LibCore/MappedFile.h>
#include <LibCore/System.h>
#include <LibJVM/JVM.h>
#include <LibMain/Main.h>

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    bool dump_class_file = false;
    StringView path;
    Core::ArgsParser parser;
    parser.add_positional_argument(path, "Path to .class or .jar file", "Path");
    parser.add_option(dump_class_file, "Dump all parsed class file structures", "dump-class-files", 'd');
    parser.parse(arguments);
    JVM::JVM jvm;
    jvm.load_from_class_file(path);
    if (dump_class_file) {
        JVM::Class loaded_class;
        loaded_class.move_from(*(jvm.resolve_class_reference(path)));
        loaded_class.dump(path);
    }
    return 0;
}
