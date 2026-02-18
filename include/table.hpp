#ifndef TABLE_HPP
#define TABLE_HPP

#include <map>
#include <memory>
#include <vector>

namespace cxx_eval
{
    template <typename CharType, typename DataType>
    class table
    {
        std::shared_ptr<DataType> data;
        std::map<CharType, table> child;
        
    public:
        using iterator = table*;
        
        ~table() = default;

        table() noexcept;
        
        table(const DataType& val);
        
        table(DataType* ptr); // 此时内存由外界管理
        
        // 数据访问
        auto get() noexcept -> DataType&;
        
        auto get_data() noexcept -> std::shared_ptr<DataType>&;

        auto get_child() noexcept -> std::map<CharType, table>&;
        
        // 设置数据
        auto set() noexcept -> void;

        auto set(const DataType& val) -> void;
        
        auto set(DataType* ptr) -> void;

        auto set(std::shared_ptr<DataType> ptr) -> void;
        
        // 判断是否有数据
        auto has_data() noexcept -> bool;
        
        // 清空子节点
        auto clear_chil() noexcept -> void;
        
        // 查找子节点
        auto find_child(const CharType& ch) noexcept -> iterator;
        
        // 添加子节点
        auto add_child(const CharType& ch) -> iterator;
        
        // 删除子节点
        auto remove_child(const CharType& ch) noexcept -> bool;
        
        // 判断是否有子节点
        auto has_children() noexcept -> bool;
        
        // 获取子节点数量
        auto child_count() noexcept -> std::size_t;

        // 添加序列
        template<typename Seq>
        auto add_seq(const Seq& seq, const DataType& val) -> iterator;
        
        template<typename Seq>
        auto add_seq(const Seq& seq, DataType* data_ptr) -> iterator;

        template<typename Seq>
        auto add_seq(const Seq& seq, std::shared_ptr<DataType> ptr) -> iterator;
        
        // 移除序列
        template<typename Seq>
        auto remove_seq(const Seq& seq) noexcept -> bool;
        
        // 查找序列
        template<typename Seq>
        auto find_seq(const Seq& seq) noexcept -> iterator;

        // 返回自身指针
        auto self() noexcept -> iterator;
    };
    
    template <typename CharType, typename DataType>
    table<CharType, DataType>::table() noexcept
        : data{nullptr}
    {
    }
    
    template <typename CharType, typename DataType>
    table<CharType, DataType>::table(const DataType& val)
        : data{std::make_shared<DataType>(val)}
    {
    }
    
    template <typename CharType, typename DataType>
    table<CharType, DataType>::table(DataType* ptr)
        : data{ptr, [](DataType*){}}
    {
    }
    
    template <typename CharType, typename DataType>
    auto table<CharType, DataType>::get() noexcept -> DataType&
    {
        return *data;
    }
    
    template <typename CharType, typename DataType>
    auto table<CharType, DataType>::get_data() noexcept -> std::shared_ptr<DataType>&
    {
        return data;
    }
    
    template <typename CharType, typename DataType>
    auto table<CharType, DataType>::get_child() noexcept -> std::map<CharType, table<CharType, DataType>>&
    {
        return child;
    }
    
    template <typename CharType, typename DataType>
    auto table<CharType, DataType>::set() noexcept -> void
    {
        data.reset();
    }
    
    template <typename CharType, typename DataType>
    auto table<CharType, DataType>::set(const DataType& val) -> void
    {
        data = std::make_shared<DataType>(val);
    }
    
    template <typename CharType, typename DataType>
    auto table<CharType, DataType>::set(DataType* ptr) -> void
    {
        data.reset(ptr, [](DataType*){});
    }

    template <typename CharType, typename DataType>
    auto table<CharType, DataType>::set(std::shared_ptr<DataType> ptr) -> void
    {
        data=ptr;
    }
    
    template <typename CharType, typename DataType>
    auto table<CharType, DataType>::has_data() noexcept -> bool
    {
        return data != nullptr;;
    }
    
    template <typename CharType, typename DataType>
    auto table<CharType, DataType>::clear_chil() noexcept -> void
    {
        child.clear();
    }
    
    template <typename CharType, typename DataType>
    auto table<CharType, DataType>::self() noexcept -> iterator
    {
        return this;
    }
    
    template <typename CharType, typename DataType>
    auto table<CharType, DataType>::find_child(const CharType& ch) noexcept -> iterator
    {
        auto it(child.find(ch));
        if (it != child.end())
            return it->second.self();
        return nullptr;
    }
    
    template <typename CharType, typename DataType>
    auto table<CharType, DataType>::add_child(const CharType& ch) -> iterator
    {
        auto result(child.emplace(ch, table{}));
        return result.first->second.self();
    }
    
    template <typename CharType, typename DataType>
    auto table<CharType, DataType>::remove_child(const CharType& ch) noexcept -> bool
    {
        return child.erase(ch);
    }
    
    template <typename CharType, typename DataType>
    auto table<CharType, DataType>::has_children() noexcept -> bool
    {
        return !child.empty();
    }
    
    template <typename CharType, typename DataType>
    auto table<CharType, DataType>::child_count() noexcept -> std::size_t
    {
        return child.size();
    }

    namespace detail
    {
        template<typename Seq>
        auto get_size(const Seq& seq) -> decltype(seq.size())
        {
            return seq.size();
        }
        
        template<std::size_t N>
        auto get_size(const char (&str)[N]) -> std::size_t
        {
            return N - 1;
        }
        
        template<std::size_t N>
        auto get_size(const wchar_t (&str)[N]) -> std::size_t
        {
            return N - 1;
        }
    }

    template <typename CharType, typename DataType>
    template<typename Seq>
    auto table<CharType, DataType>::add_seq(const Seq& seq, const DataType& val) -> iterator
    {
        auto node(this);
        typename std::map<CharType, table>::iterator it;
        
        auto size(detail::get_size(seq));
        for (std::size_t i(0); i < size; ++i)
        {
            it = node->child.find(seq[i]);
            if (it != node->child.end())
                node = it->second.self();
            else
            {
                auto result(node->child.emplace(seq[i], table{}));
                node = result.first->second.self();
            }
        }
        
        if (!node->data)
            node->data = std::make_shared<DataType>(val);
            
        return node->self();
    }
    
    template <typename CharType, typename DataType>
    template<typename Seq>
    auto table<CharType, DataType>::add_seq(const Seq& seq, DataType* data_ptr) -> iterator
    {
        auto node(this);
        typename std::map<CharType, table>::iterator it;
        
        auto size(detail::get_size(seq));
        for (std::size_t i(0); i < size; ++i)
        {
            it = node->child.find(seq[i]);
            if (it != node->child.end())
                node = it->second.self();
            else
            {
                auto result(node->child.emplace(seq[i], table{}));
                node = result.first->second.self();
            }
        }
        
        if (!node->data)
            node->data = std::shared_ptr<DataType>(data_ptr, [](DataType*){});
            
        return node->self();
    }

    template <typename CharType, typename DataType>
    template<typename Seq>
    auto table<CharType, DataType>::add_seq(const Seq& seq, std::shared_ptr<DataType> data_ptr) -> iterator
    {
        auto node(this);
        typename std::map<CharType, table>::iterator it;
        
        auto size(detail::get_size(seq));
        for (std::size_t i(0); i < size; ++i)
        {
            it = node->child.find(seq[i]);
            if (it != node->child.end())
                node = it->second.self();
            else
            {
                auto result(node->child.emplace(seq[i], table{}));
                node = result.first->second.self();
            }
        }
        
        if (!node->data)
            node->data = data_ptr;
            
        return node->self();
    }
    
    template <typename CharType, typename DataType>
    template<typename Seq>
    auto table<CharType, DataType>::remove_seq(const Seq& seq) noexcept -> bool
    {
        auto node(this);
        std::vector<std::pair<table*, CharType>> path;
        
        auto size(detail::get_size(seq));
        for (std::size_t i(0); i < size; ++i)
        {
            auto it(node->child.find(seq[i]));
            if (it == node->child.end())
                return false;
                
            path.emplace_back(node, seq[i]);
            node = it->second.self();
        }
        
        node->set();
        for (auto it(path.rbegin()); it != path.rend(); ++it)
        {
            auto& parent(it->first);
            auto ch(it->second);
            
            auto child_it(parent->child.find(ch));
            if (child_it != parent->child.end())
            {
                auto& child_node(child_it->second);
                if (child_node.has_data() || child_node.has_children())
                    parent->child.erase(ch);
                else
                    break;
            }
        }
        
        return true;
    }
    
    template <typename CharType, typename DataType>
    template<typename Seq>
    auto table<CharType, DataType>::find_seq(const Seq& seq) noexcept -> iterator
    {
        auto node(this);
        
        auto size(detail::get_size(seq));
        for (std::size_t i(0); i < size; ++i)
        {
            auto it(node->child.find(seq[i]));
            if (it == node->child.end())
                return nullptr;
                
            node = it->second.self();
        }
        
        return node->self();
    }
}

#endif