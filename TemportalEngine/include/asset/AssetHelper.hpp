#pragma once

#define DECLARE_SERIALIZATION_METHOD(method, archiveType, post) void method(archiveType &archive) post;
#define CREATE_DEFAULT_SERIALIZATION_DEFINITION(post, classAndMethod, archiveType, rootMethod) void classAndMethod(archiveType &archive) post { rootMethod(archive); }
