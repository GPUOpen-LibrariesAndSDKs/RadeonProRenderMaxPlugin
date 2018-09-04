/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Load and save Radeon ProRender XML material files
*********************************************************************************************************************************/

#include "MaterialLoader.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <stack>
#include <regex>

#include "Common.h"

using namespace std;

//executed RPR func and check for an error
#define CHECK_NO_ERROR(func) { \
	rpr_int status = func; \
	if (status != RPR_SUCCESS) \
		throw std::runtime_error("Radeon ProRender error (" + std::to_string(status) + ") in " + std::string(__FILE__) + ":" + std::to_string(__LINE__)); \
	}

namespace
{
    const std::string kTab = "    "; // default 4spaces tab for xml writer
    const int kVersion = RPR_API_VERSION;
    const std::map<int, std::string> kMaterialTypeNames{ { RPR_MATERIAL_NODE_DIFFUSE, "DIFFUSE" },
    { RPR_MATERIAL_NODE_MICROFACET, "MICROFACET" },
    { RPR_MATERIAL_NODE_REFLECTION, "REFLECTION" },
    { RPR_MATERIAL_NODE_REFRACTION, "REFRACTION" },
    { RPR_MATERIAL_NODE_MICROFACET_REFRACTION, "MICROFACET_REFRACTION" },
    { RPR_MATERIAL_NODE_TRANSPARENT, "TRANSPARENT" },
    { RPR_MATERIAL_NODE_EMISSIVE, "EMISSIVE" },
    { RPR_MATERIAL_NODE_WARD, "WARD" },
    { RPR_MATERIAL_NODE_ADD, "ADD" },
    { RPR_MATERIAL_NODE_BLEND, "BLEND" },
    { RPR_MATERIAL_NODE_ARITHMETIC, "ARITHMETIC" },
    { RPR_MATERIAL_NODE_FRESNEL, "FRESNEL" },
    { RPR_MATERIAL_NODE_NORMAL_MAP, "NORMAL_MAP" },
    { RPR_MATERIAL_NODE_IMAGE_TEXTURE, "IMAGE_TEXTURE" },
    { RPR_MATERIAL_NODE_NOISE2D_TEXTURE, "NOISE2D_TEXTURE" },
    { RPR_MATERIAL_NODE_DOT_TEXTURE, "DOT_TEXTURE" },
    { RPR_MATERIAL_NODE_GRADIENT_TEXTURE, "GRADIENT_TEXTURE" },
    { RPR_MATERIAL_NODE_CHECKER_TEXTURE, "CHECKER_TEXTURE" },
    { RPR_MATERIAL_NODE_CONSTANT_TEXTURE, "CONSTANT_TEXTURE" },
    { RPR_MATERIAL_NODE_INPUT_LOOKUP, "INPUT_LOOKUP" },
    { RPR_MATERIAL_NODE_STANDARD, "STANDARD" },
    { RPR_MATERIAL_NODE_BLEND_VALUE, "BLEND_VALUE" },
    { RPR_MATERIAL_NODE_PASSTHROUGH, "PASSTHROUGH" },
    { RPR_MATERIAL_NODE_ORENNAYAR, "ORENNAYAR" },
    { RPR_MATERIAL_NODE_FRESNEL_SCHLICK, "FRESNEL_SCHLICK" },
    { RPR_MATERIAL_NODE_DIFFUSE_REFRACTION, "DIFFUSE_REFRACTION" },
    { RPR_MATERIAL_NODE_BUMP_MAP, "BUMP_MAP" }, };

    const std::map<std::string, int> kNodeTypesMap = { { "INPUT_COLOR4F", RPR_MATERIAL_NODE_INPUT_TYPE_FLOAT4 },
    { "INPUT_FLOAT1", RPR_MATERIAL_NODE_INPUT_TYPE_FLOAT4 },
    { "INPUT_UINT", RPR_MATERIAL_NODE_INPUT_TYPE_UINT },
    { "INPUT_NODE", RPR_MATERIAL_NODE_INPUT_TYPE_NODE },
    { "INPUT_TEXTURE", RPR_MATERIAL_NODE_INPUT_TYPE_IMAGE },
    { "DIFFUSE", RPR_MATERIAL_NODE_DIFFUSE },
    { "MICROFACET", RPR_MATERIAL_NODE_MICROFACET },
    { "REFLECTION", RPR_MATERIAL_NODE_REFLECTION },
    { "REFRACTION", RPR_MATERIAL_NODE_REFRACTION },
    { "MICROFACET_REFRACTION", RPR_MATERIAL_NODE_MICROFACET_REFRACTION },
    { "TRANSPARENT", RPR_MATERIAL_NODE_TRANSPARENT },
    { "EMISSIVE", RPR_MATERIAL_NODE_EMISSIVE },
    { "WARD", RPR_MATERIAL_NODE_WARD },
    { "ADD", RPR_MATERIAL_NODE_ADD },
    { "BLEND", RPR_MATERIAL_NODE_BLEND },
    { "ARITHMETIC", RPR_MATERIAL_NODE_ARITHMETIC },
    { "FRESNEL", RPR_MATERIAL_NODE_FRESNEL },
    { "NORMAL_MAP", RPR_MATERIAL_NODE_NORMAL_MAP },
    { "IMAGE_TEXTURE", RPR_MATERIAL_NODE_IMAGE_TEXTURE },
    { "NOISE2D_TEXTURE", RPR_MATERIAL_NODE_NOISE2D_TEXTURE },
    { "DOT_TEXTURE", RPR_MATERIAL_NODE_DOT_TEXTURE },
    { "GRADIENT_TEXTURE", RPR_MATERIAL_NODE_GRADIENT_TEXTURE },
    { "CHECKER_TEXTURE", RPR_MATERIAL_NODE_CHECKER_TEXTURE },
    { "CONSTANT_TEXTURE", RPR_MATERIAL_NODE_CONSTANT_TEXTURE },
    { "INPUT_LOOKUP", RPR_MATERIAL_NODE_INPUT_LOOKUP },
    { "STANDARD", RPR_MATERIAL_NODE_STANDARD },
    { "BLEND_VALUE", RPR_MATERIAL_NODE_BLEND_VALUE },
    { "PASSTHROUGH", RPR_MATERIAL_NODE_PASSTHROUGH },
    { "ORENNAYAR", RPR_MATERIAL_NODE_ORENNAYAR },
    { "FRESNEL_SCHLICK", RPR_MATERIAL_NODE_FRESNEL_SCHLICK },
    { "DIFFUSE_REFRACTION", RPR_MATERIAL_NODE_DIFFUSE_REFRACTION },
    { "BUMP_MAP", RPR_MATERIAL_NODE_BUMP_MAP }, };

    void printMaterialNode(rpr_material_node node)
    {
        std::cout << "__________________________________________" << std::endl;

        size_t mat_name_size = 0;
        CHECK_NO_ERROR(rprMaterialNodeGetInfo(node, RPR_OBJECT_NAME, 0, nullptr, &mat_name_size));
        std::string mat_name;
        mat_name.resize(mat_name_size - 1); // std::string contains terminator already
        CHECK_NO_ERROR(rprMaterialNodeGetInfo(node, RPR_OBJECT_NAME, mat_name_size, &mat_name[0], nullptr));

        rpr_int type = 0;
        CHECK_NO_ERROR(rprMaterialNodeGetInfo(node, RPR_MATERIAL_NODE_TYPE, sizeof(rpr_int), &type, nullptr));
        std::cout << "name " << mat_name + ", type: " << kMaterialTypeNames.at(type) << std::endl;
        size_t count = 0;
        CHECK_NO_ERROR(rprMaterialNodeGetInfo(node, RPR_MATERIAL_NODE_INPUT_COUNT, sizeof(size_t), &count, nullptr));
        for (int i = 0; i < count; ++i)
        {
            size_t str_size = 0;
            CHECK_NO_ERROR(rprMaterialNodeGetInputInfo(node, i, RPR_MATERIAL_NODE_INPUT_NAME_STRING, NULL, nullptr, &str_size));
            char* str = new char[str_size];
            CHECK_NO_ERROR(rprMaterialNodeGetInputInfo(node, i, RPR_MATERIAL_NODE_INPUT_NAME_STRING, str_size, str, nullptr));
            std::cout << "\tparam: " << str << ". ";
            delete[]str; str = nullptr;
            std::cout << std::endl;
        }

        std::cout << "__________________________________________" << std::endl;
    }

    struct Param
    {
        std::string type;
        std::string value;
    };
    struct MaterialNode
    {
        std::string type;
        std::map<std::string, Param> params;
        void* data; // RPR object
    };

    class XmlWriter
    {
    public:
        XmlWriter(const std::string& file)
            : m_doc(file)
            , top_written(true)
        {

        }
        ~XmlWriter()
        {
            endDocument();
        }
        //write header
        void startDocument()
        {
            m_doc << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
        }
        void endDocument()
        {
            int size = int_cast(m_nodes.size());
            for (int i = 0; i < size; ++i)
            {
                endElement();
            }
        }
        void startElement(const std::string& node_name)
        {
            // write prev node with atts
            if (m_nodes.size() != 0 && top_written)
            {
                std::string tab = "";
                for (int i = 0; i < m_nodes.size() - 1; ++i) tab += kTab;
                const Node& node = m_nodes.top();
                m_doc << tab << "<" << node.name << "";
                for (const auto& at : node.atts)
                {
                    m_doc << " " << at.type << "=\"" << at.value << "\""; // name = "value"
                }
                m_doc << ">" << endl;
            }

            m_nodes.push({ node_name });
            top_written = true;
        }

        void endElement()
        {
            std::string tab = "";
            for (int i = 0; i < m_nodes.size() - 1; ++i) tab += kTab;
            const Node& node = m_nodes.top();
            if (top_written)
            {

                const Node& node = m_nodes.top();
                m_doc << tab << "<" << node.name << "";
                for (const auto& at : node.atts)
                {
                    m_doc << " " << at.type << "=\"" << at.value << "\""; // name = "value"
                }
                m_doc << "/>" << endl;
            }
            else
                m_doc << tab << "</" << node.name << ">" << endl;

            m_nodes.pop();
            top_written = false;
        }

        void writeAttribute(const std::string& name, const std::string& value)
        {
            Node& node = m_nodes.top();
            node.atts.push_back({ name, value });
        }

        void writeTextElement(const std::string& name, const std::string& text)
        {
            std::string tab = "";
            for (int i = 0; i < m_nodes.size() - 1; ++i) tab += kTab;
            if (m_nodes.size() != 0 && top_written)
            {
                const Node& node = m_nodes.top();
                m_doc << tab << "<" << node.name << "";
                for (const auto& at : node.atts)
                {
                    m_doc << " " << at.type << "=\"" << at.value << "\""; // name = "value"
                }
                m_doc << ">" << endl;
            }
            tab += kTab;
            
			// <name>text</name>
            m_doc << tab << "<" << name << ">" << text << "</" << name << ">" << endl;
            top_written = false;

        }
    private:
        std::ofstream m_doc;
        struct Node
        {
            std::string name;
            std::vector<Param> atts;
        };
        std::stack<Node> m_nodes;
        bool top_written; // true when element in top of m_nodes stack already written into xml
    };

    class XmlReader
    {
    public:
        struct Node
        {
            std::string name;
            std::string text;
            std::map<string, string> atts;
            bool is_closing;
            Node() : name(""), text(""), is_closing(false) {};
        };

        XmlReader(const std::string& file) noexcept
            : m_xml_text("")
            , m_is_open(false)
            , m_is_end(false)
            , m_self_closing(false)
        {
            std::ifstream doc(file);
            m_is_open = doc.is_open();
            m_is_end = !m_is_open;
            if (m_is_open)
            {
                m_xml_text = std::move(std::string((std::istreambuf_iterator<char>(doc)), std::istreambuf_iterator<char>()));
                next();
            }
        }

        bool isOpen() const noexcept
        {
            return m_is_open;
        }

        bool isEnd() const noexcept
        {
            return m_is_end;
        }
        bool next() noexcept
        {
            // root element is closed
            if (!m_is_open || m_is_end)
                return false;

            if (m_self_closing)
            {
                m_self_closing = false;
                Node& node = m_nodes.top();
                node.is_closing = true;
                return true;
            }

            try
            {
                // remove last node if it's closed
                if (!m_nodes.empty() && m_nodes.top().is_closing)
                    m_nodes.pop();

                // regex for searching XML nodes
                std::regex node_reg("<[^<^>]*>");
                std::smatch node_match;
                m_is_end = !std::regex_search(m_xml_text, node_match, node_reg);
                if (m_is_end)
                    return !m_is_end;

                if (node_match.size() == 0)
                    Throw("Invalid xml: bad node");

                std::string value = node_match[0];
                std::regex name_reg("[^<][^>\\s/]+"); //ignoring '<' symbol
                std::smatch name_match;
                std::regex_search(value, name_match, name_reg);

                // create new node if this is not the closing one
                if (name_match.str().find('/') == std::string::npos)
                    m_nodes.push(Node());

                Node& node = m_nodes.top();
                node.name = name_match.str();
                const std::string ksc_ending = "/>"; //self-closing node ending
                if (value.size() > ksc_ending.size())
                    m_self_closing = std::equal(ksc_ending.rbegin(), ksc_ending.rend(), value.rbegin());

                if (node.name.find('/') != std::string::npos)
                {
                    node.is_closing = true;
                    node.name.erase(std::remove(node.name.begin(), node.name.end(), '/')); //remove closing '/'
                }
                std::regex att_split_reg("[\\S]+=\".*?\""); //split attributes regex string="string"
                std::string attributes = name_match.suffix().str();
                auto att_split_begin = std::sregex_iterator(attributes.begin(), attributes.end(), att_split_reg);
                auto att_split_end = std::sregex_iterator();

                // parsing attributes
                for (auto i = att_split_begin; i != att_split_end; ++i)
                {
                    std::string att = i->str();
                    std::regex att_reg("[^=?>\"]+");
                    auto att_begin = std::sregex_iterator(att.begin(), att.end(), att_reg);
                    auto att_end = std::sregex_iterator();
                    std::vector<std::string> splited_att;
                    splited_att.reserve(2); //should be 2 values
                    for (auto j = att_begin; j != att_end; ++j)
                    {
                        splited_att.push_back(j->str());
                    }

                    if (splited_att.size() == 2)
                        node.atts[splited_att[0]] = splited_att[1];
                    else // case when value = ""
                        node.atts[splited_att[0]] = "";
                }
                
				// cut parsed data
                m_xml_text = node_match.suffix().str();
                
				// looking for node text data
                size_t pos = m_xml_text.find('<');
                if (pos != std::string::npos && !m_self_closing)
                {
                    node.text = m_xml_text.substr(0, pos);
                    node.text = std::regex_replace(node.text, std::regex("^[\\s]+"), ""); //removing beginning whitespaces
                    node.text = std::regex_replace(node.text, std::regex("[\\s]+^"), ""); //removing ending whitespaces
                    node.text = std::regex_replace(node.text, std::regex("[\\s]+"), " "); //replace multiple whitespaces by single space
                }
            }
            catch (std::exception e)
            {
                cout << "Regex exception: " << e.what() << endl;
                m_is_end = true;
            }
            return !m_is_end;
        }

        const Node& get() const
        {
            return m_nodes.top();
        }
    private:
        void Throw(const std::string& msg)
        {
            m_is_end = true;
            throw std::runtime_error(msg);
        }
        std::string m_xml_text;
        bool m_is_open;
        bool m_is_end;
        bool m_self_closing;
        std::stack<Node> m_nodes; //stores previoulsy open yet not closed nodes
    };

    rpr_material_node CreateMaterial(rpr_material_system sys, const MaterialNode& node, const std::string& name)
    {
        rpr_material_node mat = nullptr;
        int type = kNodeTypesMap.at(node.type);
        CHECK_NO_ERROR(rprMaterialSystemCreateNode(sys, type, &mat));
        CHECK_NO_ERROR(rprObjectSetName(mat, name.c_str()));
        return mat;
    }
}

void ExportMaterials(const std::string& filename,
	const std::set<rpr_material_node>& nodeList,
	const std::set<rprx_material>& nodeListX ,
	const std::vector<RPRX_DEFINE_PARAM_MATERIAL>& rprxParamList,
	rprx_context contextX ,
	std::unordered_map<rpr_image , RPR_MATERIAL_XML_EXPORT_TEXTURE_PARAM>& textureParameter,
	void* closureNode,  // pointer to a rpr_material_node or a rprx_material
	const std::string& material_name // example : "Emissive_Fluorescent_Magenta"
	)
{
    XmlWriter writer(filename);
    if (nodeListX.size() == 0 && nodeList.size() == 0)
    {
        cout << "MaterialExport error: materials input array is nullptr" << endl;
        return;
    }

    try
    {
        writer.startDocument();
        writer.startElement("material");
        writer.writeAttribute("name", material_name);
        
		// in case we need versioning the material XMLs...  -  unused for the moment.
		writer.writeAttribute("version_exporter", "10"); 

		//version of the RPR_API_VERSION used to generate this XML
		std::stringstream hex_version;
        hex_version << "0x" << std::hex << kVersion;
        writer.writeAttribute("version_rpr", hex_version.str());
		
        // file path as key, id - for material nodes connections
        std::map<std::string,  std::pair< std::string , rpr_image >  > textures;
        std::map<void*, std::string> objects; // <object, node_name> map

        std::set<std::string> used_names; // to avoid duplicating node names

		std::string closureNodeName = "";

		// fill materials first
		for(const auto& iNode : nodeList )
        {
            rpr_material_node mat = iNode;
            if (objects.count(mat))
                continue;
            size_t name_size = 0;
            CHECK_NO_ERROR(rprMaterialNodeGetInfo(mat, RPR_OBJECT_NAME, NULL, nullptr, &name_size));
            std::string mat_node_name;
            mat_node_name.resize(name_size - 1); // exclude extra terminator
            CHECK_NO_ERROR(rprMaterialNodeGetInfo(mat, RPR_OBJECT_NAME, name_size, &mat_node_name[0], nullptr));
            int postfix_id = 0;

            if (mat_node_name.empty()) 
				mat_node_name = "node" + std::to_string(postfix_id);

            while (used_names.count(mat_node_name))
            {
                mat_node_name = "node" + std::to_string(postfix_id);
                ++postfix_id;
            }

			if ( mat == closureNode )
					closureNodeName = mat_node_name;

            objects[mat] = mat_node_name;
            used_names.insert(mat_node_name);
        }

		for(const auto& iNode : nodeListX )
		{
			rprx_material mat = iNode;
			if (objects.count(mat))
			continue;

			int postfix_id = 0;
			std::string mat_node_name = "Uber_" + std::to_string(postfix_id);

			while (used_names.count(mat_node_name))
            {
                mat_node_name = "Uber_" + std::to_string(postfix_id);
                ++postfix_id;
            }

			if ( mat == closureNode )
					closureNodeName = mat_node_name;
			
			objects[mat] = mat_node_name;
			used_names.insert(mat_node_name);
		}

		// closure_node is the name of the node containing the final output of the material
		writer.writeAttribute("closure_node", closureNodeName); 

        // look for images
		for(const auto& iNode : nodeList )
        {
            rpr_material_node mat = iNode;
            size_t input_count = 0;
            CHECK_NO_ERROR(rprMaterialNodeGetInfo(mat, RPR_MATERIAL_NODE_INPUT_COUNT, sizeof(size_t), &input_count, nullptr));
            for (int input_id = 0; input_id < input_count; ++input_id)
            {
                rpr_material_node_type input_type;
                CHECK_NO_ERROR(rprMaterialNodeGetInputInfo(mat, input_id, RPR_MATERIAL_NODE_INPUT_TYPE, sizeof(input_type), &input_type, nullptr));
                if (input_type != RPR_MATERIAL_NODE_INPUT_TYPE_IMAGE)
                    continue;

                rpr_image img = nullptr;
                CHECK_NO_ERROR(rprMaterialNodeGetInputInfo(mat, input_id, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(img), &img, nullptr));
                if (objects.count(img)) //already mentioned
                    continue;

                std::string img_node_name = "node0";
                int postfix_id = 0;
                while (used_names.count(img_node_name))
                {
                    img_node_name = "node" + std::to_string(postfix_id);
                    ++postfix_id;
                }
                objects[img] = img_node_name;
                used_names.insert(img_node_name);
            }
        }

        // optionally write description (at the moment there is no description)
        writer.writeTextElement("description", "");

		for(const auto& iNode : nodeList )
        {
            rpr_material_node node = iNode;

            rpr_material_node_type type;
            size_t input_count = 0;
            CHECK_NO_ERROR(rprMaterialNodeGetInfo(node, RPR_MATERIAL_NODE_TYPE, sizeof(rpr_material_node_type), &type, nullptr));
            CHECK_NO_ERROR(rprMaterialNodeGetInfo(node, RPR_MATERIAL_NODE_INPUT_COUNT, sizeof(size_t), &input_count, nullptr));
            writer.startElement("node");
            writer.writeAttribute("name", objects[node]);
            writer.writeAttribute("type", kMaterialTypeNames.at(type));
            for (int input_id = 0; input_id < input_count; ++input_id)
            {
                size_t read_count = 0;
                CHECK_NO_ERROR(rprMaterialNodeGetInputInfo(node, input_id, RPR_MATERIAL_NODE_INPUT_NAME_STRING, sizeof(size_t), nullptr, &read_count));
                std::vector<char> param_name(read_count, ' ');
                CHECK_NO_ERROR(rprMaterialNodeGetInputInfo(node, input_id, RPR_MATERIAL_NODE_INPUT_NAME_STRING, sizeof(param_name), param_name.data(), nullptr));
                std::string type = "";
                std::string value = "";
                rpr_material_node_type input_type;
                CHECK_NO_ERROR(rprMaterialNodeGetInputInfo(node, input_id, RPR_MATERIAL_NODE_INPUT_TYPE, sizeof(input_type), &input_type, &read_count));
                switch (input_type)
                {
                case RPR_MATERIAL_NODE_INPUT_TYPE_FLOAT4:
                {
                    rpr_float fvalue[4] = { 0.f, 0.f, 0.f, 0.f };
                    CHECK_NO_ERROR(rprMaterialNodeGetInputInfo(node, input_id, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(fvalue), fvalue, nullptr));
                    //fvalue converted to string
                    std::stringstream ss;
                    ss << fvalue[0] << ", " <<
                        fvalue[1] << ", " <<
                        fvalue[2] << ", " <<
                        fvalue[3];
                    type = "float4";
                    value = ss.str();
                    break;
                }
                case RPR_MATERIAL_NODE_INPUT_TYPE_UINT:
                {
                    rpr_int uivalue = RPR_SUCCESS;
                    CHECK_NO_ERROR(rprMaterialNodeGetInputInfo(node, input_id, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(uivalue), &uivalue, nullptr));
                    
					//value converted to string
                    type = "uint";
                    value = std::to_string(uivalue);
                    break;
                }
                case RPR_MATERIAL_NODE_INPUT_TYPE_NODE:
                {
                    rpr_material_node connection = nullptr;
                    rpr_int res = rprMaterialNodeGetInputInfo(node, input_id, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(connection), &connection, nullptr);
                    CHECK_NO_ERROR(rprMaterialNodeGetInputInfo(node, input_id, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(connection), &connection, nullptr));
                    type = "connection";
                    if (!objects.count(connection) && connection)
                    {
                        throw std::runtime_error("input material node is missing");
                    }
                    value = objects[connection];
                    break;
                }
                case RPR_MATERIAL_NODE_INPUT_TYPE_IMAGE:
                {
                    type = "connection";
                    rpr_image tex = nullptr;
                    CHECK_NO_ERROR(rprMaterialNodeGetInputInfo(node, input_id, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(tex), &tex, nullptr));
                    size_t name_size = 0;
                    CHECK_NO_ERROR(rprImageGetInfo(tex, RPR_OBJECT_NAME, NULL, nullptr, &name_size));
                    std::string tex_name;
                    tex_name.resize(name_size - 1);
                    CHECK_NO_ERROR(rprImageGetInfo(tex, RPR_OBJECT_NAME, name_size, &tex_name[0], nullptr));


					//replace \\ by /
					//so we ensure all paths are using same convention  ( better for Unix )
					for(int i=0; i<tex_name.length(); i++)
					{
						if ( tex_name[i] == '\\' )
							tex_name[i] = '/';
					}


					// we don't want path that looks like :  "H:/RPR/MatLib/MaterialLibrary/1.0 - Copy/RadeonProRMaps/Glass_Used_normal.jpg"
					// all paths must look like "RadeonProRMaps/Glass_Used_normal.jpg"
					// the path RadeonProRMaps/  is hard coded here for the moment .. needs to be exposed in exporter GUI if we change it ?
					const std::string radeonProRMapsFolder = "/RadeonProRMaps/";
					size_t pos = tex_name.find(radeonProRMapsFolder);
					if ( pos != std::string::npos )
					{
						tex_name = tex_name.substr(pos+1);
						int a=0;
					}



                    if (!textures.count(tex_name))
                    {
                        int tex_node_id = 0;
                        std::string tex_node_name = objects[tex];
                        textures[tex_name] = std::pair<std::string , rpr_image> ( tex_node_name , tex ) ;
                    }
                    value = textures[tex_name].first;
                    break;
                }
                default:
                    throw std::runtime_error("unexpected material node input type " + std::to_string(input_type));
                }
                if (!value.empty())
                {
                    writer.startElement("param");
                    writer.writeAttribute("name", param_name.data());
                    writer.writeAttribute("type", type);
                    writer.writeAttribute("value", value);
                    writer.endElement();
                }
            }
            writer.endElement();
        }


		for(const auto& iNode : nodeListX )
        {
			rprx_material materialX = iNode;

			writer.startElement("node");
            writer.writeAttribute("name", objects[materialX]);
            writer.writeAttribute("type", "UBER");

			for(int iParam=0; iParam<rprxParamList.size(); iParam++)
			{
				std::string type = "";
                std::string value = "";

				rprx_parameter_type input_type = 0;
				CHECK_NO_ERROR( rprxMaterialGetParameterType(contextX,materialX,rprxParamList[iParam].param,&input_type));

				if ( input_type == RPRX_PARAMETER_TYPE_FLOAT4 )
				{
					rpr_float fvalue[4] = { 0.f, 0.f, 0.f, 0.f };
					CHECK_NO_ERROR( rprxMaterialGetParameterValue(contextX,materialX,rprxParamList[iParam].param,&fvalue) );

                    //fvalue converted to string
                    std::stringstream ss;
                    ss << fvalue[0] << ", " <<
                        fvalue[1] << ", " <<
                        fvalue[2] << ", " <<
                        fvalue[3];
                    type = "float4";
                    value = ss.str();


				}
				else if ( input_type == RPRX_PARAMETER_TYPE_UINT )
				{

					rpr_int uivalue = RPR_SUCCESS;
					
                    CHECK_NO_ERROR( rprxMaterialGetParameterValue(contextX,materialX,rprxParamList[iParam].param,&uivalue));

					//value converted to string
                    type = "uint";
                    value = std::to_string(uivalue);


				}
				else if ( input_type == RPRX_PARAMETER_TYPE_NODE )
				{
					rpr_material_node connection = 0;
					CHECK_NO_ERROR( rprxMaterialGetParameterValue(contextX,materialX,rprxParamList[iParam].param,&connection));

					if ( connection )
					{
						type = "connection";
						if (!objects.count(connection) && connection)
						{
							throw std::runtime_error("input material node is missing");
						}
						const auto& connectName = objects.find(connection);
						if ( connectName != objects.end() )
						{
							value = connectName->second;
						}
						else
						{
							int a=0; // ERROR ?
						}
					}
				}
				else
				{
					throw std::runtime_error("unexpected material nodeX input type " + std::to_string(input_type));
                }
                if (!value.empty())
                {
                    writer.startElement("param");
                    writer.writeAttribute("name", rprxParamList[iParam].nameInXML  );
                    writer.writeAttribute("type", type);
                    writer.writeAttribute("value", value);
                    writer.endElement();
                }

			}

			writer.endElement();
		}



        for (const auto& tex : textures)
        {
            writer.startElement("node");
            writer.writeAttribute("name", tex.second.first);
            writer.writeAttribute("type", "INPUT_TEXTURE");
            writer.startElement("param");
            writer.writeAttribute("name", "path");
            writer.writeAttribute("type", "file_path");
            writer.writeAttribute("value", tex.first);
            writer.endElement();

			writer.startElement("param");
			writer.writeAttribute("name", "gamma");
			writer.writeAttribute("type", "float");
			bool gammaset = false;
			if ( textureParameter.find(tex.second.second) != textureParameter.end()  )
			{
				RPR_MATERIAL_XML_EXPORT_TEXTURE_PARAM& param = textureParameter[tex.second.second];
				if ( param.useGamma )
				{
					writer.writeAttribute("value", std::to_string(2.2f) );
					gammaset = true;
				}
			}
			else
			{
				int a=0;
			}
			if ( !gammaset )
			{
				writer.writeAttribute("value", std::to_string(1.0f) );
			}
			writer.endElement();

            writer.endElement();
        }
        writer.endDocument();
    }
    catch (const std::exception& ex)
    {
        cout << "MaterialExport error: " << ex.what() << endl;
    }

	return;
}

bool ImportMaterials(const std::string& filename, rpr_context context, rpr_material_system sys, rpr_material_node** out_materials, int* out_mat_count)
{
    XmlReader read(filename);
    if (!read.isOpen())
    {
        std::cout << "Failed to open file " << filename << std::endl;
        return false;
    }
    std::map<std::string, MaterialNode> nodes;
    MaterialNode* last_node = nullptr;
    std::vector<rpr_material_node> material_nodes;
    std::vector<rpr_image> textures;
    std::vector<std::string> tex_files;

    try
    {
        while (!read.isEnd())
        {
            const XmlReader::Node& node = read.get();
            if (!node.is_closing)
            {
                if (node.name == "node")
                {
                    last_node = &(nodes[read.get().atts.at("name")]);
                    last_node->type = read.get().atts.at("type");
                }
                else if (node.name == "param")
                {
                    std::string param_name = read.get().atts.at("name");
                    std::string param_type = read.get().atts.at("type");
                    std::string param_value = read.get().atts.at("value");
                    last_node->params[param_name] = { param_type, param_value };
                }
                else if (node.name == "description")
                {
					// at the moment we don't support a description field. easy to add:
                    // description = read.get().text;
                }
                else if (node.name == "material")
                {
                    std::stringstream version_stream;
                    version_stream << std::hex << node.atts.at("version");
                    int version;
                    version_stream >> version;
                    if (version != kVersion)
                        std::cout << "Warning: Invalid API version. Expected " << hex << kVersion << "." << std::endl;
                }
            }
            read.next();
        }

        // count material nodes
        int count = 0;
        for (auto& i : nodes)
            if (i.second.type.find("INPUT") == std::string::npos)
                ++count;

        // create nodes
        for (auto& i : nodes)
        {
            MaterialNode& node = i.second;
            if (node.type.find("INPUT") == std::string::npos)
            {
                material_nodes.push_back(CreateMaterial(sys, i.second, i.first));
                node.data = material_nodes.back();
            }
            else if (node.type == "INPUT_TEXTURE")
            {
                rpr_image texture = nullptr;
                const std::string kFilename = node.params.at("path").value;
                CHECK_NO_ERROR(rprContextCreateImageFromFile(context, kFilename.c_str(), &texture));
                textures.push_back(texture);
                tex_files.push_back(kFilename);

                node.data = texture;
            }
        }

        // setup material parameters
        for (auto& i : nodes)
        {
            MaterialNode& node = i.second;

            if (!node.data || node.type == "INPUT_TEXTURE") // we only need material nodes
                continue;
            for (auto param : node.params)
            {
                std::string in_input = param.first;
                std::string type = param.second.type;
                MaterialNode* data_node = &node;
                Param* data_param = &param.second;
                if (type == "connection")
                {
                    data_node = &nodes.at(param.second.value); // connection node
                    if (data_node->type.find("INPUT") != std::string::npos && data_node->params.size() == 1)
                        data_param = &(data_node->params.begin()->second);
                    else if (!data_node->data)
                    {
                        cout << "Error: invalid connection.\n";
                        return false;
                    }
                }

                if (data_node->type == "INPUT_TEXTURE")
                {
                    CHECK_NO_ERROR(rprMaterialNodeSetInputImageData(node.data, in_input.c_str(), data_node->data));
                }
                else if (data_param->type.find("float") != std::string::npos)
                {
                    rpr_float fvalue[4] = { 0.f, 0.f, 0.f, 0.f };
                    std::string data = data_param->value;
                    std::replace(data.begin(), data.end(), ',', ' '); // replace comas by spaces to simplify string splitting

                    std::istringstream iss(data);
                    std::vector<std::string> tokens{ istream_iterator<std::string>{iss}, std::istream_iterator<string>{} };
                    int i = 0;
                    for (const auto& val : tokens)
                    {
                        fvalue[i] = std::stof(val);
                        ++i;
                    }
                    CHECK_NO_ERROR(rprMaterialNodeSetInputF(node.data, in_input.c_str(), fvalue[0], fvalue[1], fvalue[2], fvalue[3]));
                }
                else if (data_node->type == "INPUT_UINT")
                {
                    // not handled
                    rpr_uint uivalue = 0;
                    CHECK_NO_ERROR(rprMaterialNodeSetInputU(node.data, in_input.c_str(), uivalue));
                }
                else if (type == "file_path")
                {
                    rpr_image img = nullptr;
                    CHECK_NO_ERROR(rprContextCreateImageFromFile(context, param.second.value.c_str(), &img));
                    CHECK_NO_ERROR(rprMaterialNodeSetInputImageData(node.data, in_input.c_str(), img));
                }
                else if (data_node->data)
                {
                    CHECK_NO_ERROR(rprMaterialNodeSetInputN(node.data, in_input.c_str(), data_node->data));
                }
                else 
                {
					// not handled
                    cout << "Warning: Unknwon node type: " << data_node->type << endl;
                }
            }
        }

        // fill out buffers
        if (textures.size() != tex_files.size())
        {
            cout << "Error: internal error." << endl;
            return false;
        }
        if (out_materials)
        {
            *out_materials = new rpr_material_node[material_nodes.size()];
            memcpy(*out_materials, material_nodes.data(), material_nodes.size() * sizeof(rpr_material_node));
        }
        if (out_mat_count)
            *out_mat_count = int_cast(material_nodes.size());
    }
    catch (const std::exception& e)
    {
        cout << "MaterialImport error: " << e.what() << endl;

		// Make log to Max Log
		string str = e.what();
		std::wstring wstr(str.begin(), str.end());
		FireRender::LogErrorStringToMaxLog(wstr);

        return false;
    }
    return true;
}
