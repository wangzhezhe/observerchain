#include "../lib/rapidjson/include/rapidjson/document.h"
#include "../lib/rapidjson/include/rapidjson/writer.h"
#include "../lib/rapidjson/include/rapidjson/stringbuffer.h"
#include <iostream>
#include "stdio.h"

using namespace rapidjson;

//if you want to put Document as a parameter, please use &
void func(const char *json, Document &d)
{
    d.Parse(json);

    // 2. Modify it by DOM.
    Value &s = d["stars"];
    s.SetInt(s.GetInt() + 1);

    //get empty value
    bool ifContain = d.HasMember("test");
    if (ifContain == false)
    {
        printf("not contain key: test\n");
    }

    // 3. Stringify the DOM
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    d.Accept(writer);

    // Output {"project":"rapidjson","stars":11}
    std::cout << buffer.GetString() << std::endl;
}

int main()
{
    // 1. Parse a JSON string into DOM.
    const char *json = "{\"project\":\"rapidjson\",\"stars\":10}";
    Document d;
    func(json, d);

    return 0;
}