#include <pugixml.hpp>
#include <iostream>
#include <string>


class xml_string_writer : public pugi::xml_writer
{
public:
  std::string result;

  virtual void write(const void* data, size_t size)
  {
      result.append(static_cast<const char*>(data), size);
      std::cout << (char *)data << std::endl;
  }
};


int main()
{
  pugi::xml_document doc;
  pugi::xml_node node = doc.append_child("stream:stream");
  node.append_attribute("to") = "tilda.center";
  node.append_attribute("xmlns") = "jabber:client";
  node.append_attribute("xml:stream") = "http://etherx.jabber.org/streams";
  node.append_attribute("version") = "1.0";

  xml_string_writer writer;
  doc.save(writer, "", pugi::format_raw | pugi::format_no_declaration);

  return 0;
}
