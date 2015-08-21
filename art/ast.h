#pragma once

#include <string>
#include <sstream>
#include <vector>

#include <regex>

///////////////////////////////
class ast_node
{
public:
    ast_node() {}
    virtual ~ast_node() {}

    virtual std::string to_string() const = 0;
};

///////////////////////////////
class ast_node_atom : public ast_node
{
};

///////////////////////////////
class ast_atom_symbol : public ast_node_atom
{
public:
    ast_atom_symbol(const std::string& a_symbol)
    : symbol(a_symbol)
    {}

    std::string to_string() const override
    {
        std::ostringstream stringStream;
        stringStream << symbol;
        return stringStream.str();
    }

private:
    std::string symbol;
};

///////////////////////////////
class ast_atom_int : public ast_node_atom
{
public:
    ast_atom_int(int a_value)
    : value(a_value)
    {}

    std::string to_string() const override
    {
        std::ostringstream stringStream;
        stringStream << value;
        return stringStream.str();
    }

private:
    int value;
};

///////////////////////////////
class ast_node_list : public ast_node
{
public:

    void add_node(std::unique_ptr<ast_node> node)
    {
        nodes.push_back(std::move(node));
    }

    std::string to_string() const override
    {
        std::ostringstream stringStream;
        stringStream << "(";
        bool addSpace = false;
        for (auto&& n : nodes)
        {
            if (addSpace)
            {
                stringStream << " ";
            }
            else
            {
                addSpace = true;
            }
            stringStream << n->to_string();
        }
        stringStream << ")";
        return stringStream.str();
    }

private:
    std::vector<std::unique_ptr<ast_node> > nodes;
};

///////////////////////////////
class ast_node_vector : public ast_node
{
public:

    void add_node(std::unique_ptr<ast_node> node)
    {
        nodes.push_back(std::move(node));
    }

    std::string to_string() const override
    {
        std::ostringstream stringStream;
        stringStream << "[";
        bool addSpace = false;
        for (auto&& n : nodes)
        {
            if (addSpace)
            {
                stringStream << " ";
            }
            else
            {
                addSpace = true;
            }
            stringStream << n->to_string();
        }
        stringStream << "]";
        return stringStream.str();
    }

private:
    std::vector<std::unique_ptr<ast_node> > nodes;
};


///////////////////////////////
class ast
{
public:
    //
    static std::unique_ptr<ast> parse(const std::string& a_line)
    {
        return std::unique_ptr<ast>(new ast(a_line));
    }

    std::string to_string() const
    {
        return root->to_string();
    }

private:
    //
    ast(const std::string& a_line);
    
    
    //
    std::unique_ptr<ast_node> root;
};
