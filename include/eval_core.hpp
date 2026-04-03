/*

    C++11 header only
    github:https://github.com/ydog01/cxx_eval
    2026/4/2 -- version 1.0

*/

#ifndef EVAL_CORE
#define EVAL_CORE

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <tuple>
#include <list>

namespace ydog01
{
    namespace core
    {
        template <typename T, typename... Args>
        std::unique_ptr<T> make_unique(Args &&...args)
        {
            return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        class Node
        {
            using DataTuple = std::tuple<std::shared_ptr<DataType>...>;
            DataTuple data;
            MapType<KeyType, Node> child;

        public:
            using iterator = Node *;

            Node() = default;
            Node(const Node &other);
            auto operator=(const Node &other) -> Node &;
            Node(Node &&other) noexcept;
            auto operator=(Node &&other) noexcept -> Node &;
            ~Node() = default;

            // Creates a deep copy of the node and all its children
            auto deep_copy() const -> Node;

            // Gets the data at index I
            template <std::size_t I>
            auto get_data() -> typename std::tuple_element<I, DataTuple>::type;

            template <std::size_t I>
            auto get_data() const -> std::shared_ptr<const typename std::tuple_element<I, DataTuple>::type::element_type>;

            // Sets the data at index I
            template <std::size_t I>
            auto set_data(typename std::tuple_element<I, DataTuple>::type value) -> void;

            // Removes the data at index I
            template <std::size_t I>
            auto remove_data() -> void;

            // Checks if data exists at index I
            template <std::size_t I>
            auto has_data() const -> bool;

            // Gets the child map
            auto get_child() -> MapType<KeyType, Node> &;
            auto get_child() const -> const MapType<KeyType, Node> &;

            // Gets the child node for the given key, returns nullptr if not found
            auto next(const KeyType &key) -> Node *;
            auto next(const KeyType &key) const -> const Node *;

            // Searches for a node following the sequence of keys (variadic)
            template <typename... Keys>
            auto search(const Keys &...keys) -> Node *;

            template <typename... Keys>
            auto search(const Keys &...keys) const -> const Node *;

            // Searches for a node following the sequence of keys (container)
            template <typename Container>
            auto search(const Container &keys) -> decltype(std::begin(keys), std::end(keys), static_cast<Node*>(nullptr));

            template <typename Container>
            auto search(const Container &keys) const -> decltype(std::begin(keys), std::end(keys), static_cast<const Node*>(nullptr));

            template <typename... Keys>
            auto insert(const Keys &...keys) -> Node *;

            template <typename Container>
            auto insert(const Container &keys) -> decltype(std::begin(keys), std::end(keys), static_cast<Node*>(nullptr));

            template <std::size_t I, typename... Keys>
            auto remove(const Keys &...keys) -> bool;

            template <std::size_t I, typename Container>
            auto remove(const Container &keys) -> decltype(std::begin(keys), std::end(keys),keys.size(), true);

            auto clear_children() -> void;
            
            auto has_any_data() const -> bool;

        private:
            template <std::size_t J>
            auto has_any_data_impl() const -> typename std::enable_if<J == sizeof...(DataType), bool>::type;

            template <std::size_t J>
                auto has_any_data_impl() const -> typename std::enable_if < J<sizeof...(DataType), bool>::type;
                
            template <std::size_t I>
            auto deep_copy_data_recursive(DataTuple &result) const -> void;
        };

        template<typename DataType>
        class ParamViewer
        {
            DataType** begin_;
            std::size_t siz;
        public:
            ParamViewer(DataType** beg, std::size_t siz_):begin_(beg),siz(siz_){}
            std::size_t size() const
            {
                return siz;
            }
            DataType& operator[](std::size_t pos)
            {
                if(pos>=siz)
                    throw std::out_of_range("Index out of range");
                return *begin_[pos];
            }
            const DataType& operator[](std::size_t pos) const
            {
                if(pos>=siz)
                    throw std::out_of_range("Index out of range");
                return *begin_[pos];
            }

            class Iterator
            {
                DataType **ptr;

            public:
                explicit Iterator(DataType **p = nullptr) : ptr(p)
                {
                }

                DataType &operator*() const
                {
                    return **ptr;
                }

                Iterator &operator++()
                {
                    ++ptr;
                    return *this;
                }

                bool operator!=(const Iterator &other) const
                {
                    return ptr != other.ptr;
                }
            };

            Iterator begin()
            {
                return Iterator(begin_);
            }
            Iterator end()
            {
                return Iterator(begin_ + siz);
            }
        };

        enum class Associativity
        {
            Left,
            Right
        };

        template<typename KeyType,typename DataType>
        struct OperatorEx;

        template<typename DataType,template<typename>class PtrType>
        struct Expression;

        template<typename KeyType,typename DataType>
        struct ParserInfo
        {
            bool value_class = true;
            std::list<std::pair<std::shared_ptr<OperatorEx<KeyType,DataType>>,std::size_t>> stack;
            Expression<DataType,std::shared_ptr> expression;
            std::size_t pos = 0;
            const std::basic_string<KeyType>& keys;
            ParserInfo(const std::basic_string<KeyType>& str) : keys(str) {}
        };

        template<typename DataType>
        struct Operator
        {
            std::function<DataType(ParamViewer<DataType>)> function;
        };

        enum class BreakType
        {
            RETURN,
            BREAK,
            NONE,
            CONTINUE
        };

        struct ExtraData
        {
        virtual ~ExtraData(){}
        };

        template<typename KeyType,typename DataType>
        struct OperatorEx:Operator<DataType>
        {
            std::int32_t precedence = 0;
            Associativity assoc = Associativity::Left;
            std::size_t default_param_size = 0;
            
            std::unique_ptr<ExtraData> extra_data;
            std::function<bool(ParserInfo<KeyType, DataType>&)> extra_front;
            std::function<BreakType(ParserInfo<KeyType, DataType>&,std::shared_ptr<OperatorEx<KeyType, DataType>>)> extra_mid;
            std::function<void(ParserInfo<KeyType, DataType>&)> extra_back;
            std::function<BreakType(ParserInfo<KeyType, DataType>&)> extra_remaining;
        };

        enum class TokenType:uint8_t
        {
            Constant,
            Variale,
            Operator,
        };

        template <typename T>
        struct is_weak_ptr : std::false_type{};

        template <typename T>
        struct is_weak_ptr<std::weak_ptr<T>> : std::true_type{};

        template <typename DataType, template <typename> class PtrType>
        struct Expression
        {
            std::list<TokenType> index;
            std::list<std::pair<PtrType<Operator<DataType>>, std::size_t>> operators;
            std::list<PtrType<DataType>> variables;
            std::list<std::unique_ptr<DataType>> constants;

            Expression(const Expression &) = delete;
            Expression &operator=(const Expression &) = delete;

            Expression() = default;

            Expression(Expression &&other) noexcept = default;
            Expression &operator=(Expression &&other) noexcept = default;

            template <template <typename> class OtherPtrType,
                      typename std::enable_if<
                          std::is_same<PtrType<DataType>, std::weak_ptr<DataType>>::value &&
                          std::is_same<OtherPtrType<DataType>, std::shared_ptr<DataType>>::value>::type * = nullptr>
            Expression(Expression<DataType, OtherPtrType> &&other) noexcept;

            template <template <typename> class OtherPtrType,
                      typename std::enable_if<
                          std::is_same<PtrType<DataType>, std::shared_ptr<DataType>>::value &&
                          std::is_same<OtherPtrType<DataType>, std::weak_ptr<DataType>>::value>::type * = nullptr>
            Expression(Expression<DataType, OtherPtrType> &&other);

            template <template <typename> class OtherPtrType,
                      typename std::enable_if<
                          std::is_same<PtrType<DataType>, std::weak_ptr<DataType>>::value &&
                          std::is_same<OtherPtrType<DataType>, std::shared_ptr<DataType>>::value>::type * = nullptr>
            Expression &operator=(Expression<DataType, OtherPtrType> &&other) noexcept;

            template <template <typename> class OtherPtrType,
                      typename std::enable_if<
                          std::is_same<PtrType<DataType>, std::shared_ptr<DataType>>::value &&
                          std::is_same<OtherPtrType<DataType>, std::weak_ptr<DataType>>::value>::type * = nullptr>
            Expression &operator=(Expression<DataType, OtherPtrType> &&other);

            template <typename U = PtrType<DataType>>
            auto value() const -> typename std::enable_if<!is_weak_ptr<U>::value, DataType>::type;

            template <typename U = PtrType<DataType>>
            auto value() const -> typename std::enable_if<is_weak_ptr<U>::value, DataType>::type;

        private:
            static auto convert_operators_shared_to_weak(
                std::list<std::pair<std::shared_ptr<Operator<DataType>>, std::size_t>> &&other_ops)
                -> std::list<std::pair<std::weak_ptr<Operator<DataType>>, std::size_t>>;

            static auto convert_operators_weak_to_shared(
                std::list<std::pair<std::weak_ptr<Operator<DataType>>, std::size_t>> &&other_ops)
                -> std::list<std::pair<std::shared_ptr<Operator<DataType>>, std::size_t>>;

            static auto convert_variables_shared_to_weak(std::list<std::shared_ptr<DataType>> &&other_vars)
                -> std::list<std::weak_ptr<DataType>>;

            static auto convert_variables_weak_to_shared(std::list<std::weak_ptr<DataType>> &&other_vars)
                -> std::list<std::shared_ptr<DataType>>;
        };

        template <template <typename, typename> class MapType, typename KeyType, typename DataType>
        struct ParserContext
        {
            using NodeType = Node<MapType,KeyType,OperatorEx<KeyType,DataType>,OperatorEx<KeyType,DataType>,OperatorEx<KeyType,DataType>,DataType>;
            static constexpr std::size_t prefix_pos = 0;
            static constexpr std::size_t infix_pos = 1;
            static constexpr std::size_t suffix_pos = 2;
            static constexpr std::size_t variable_pos = 3;

            NodeType resource;
            std::function<bool(ParserInfo<KeyType, DataType>&)> skip;//if pos==size => return true
            std::function<std::unique_ptr<DataType>(ParserInfo<KeyType, DataType>&)> constant_parser;//On failure, roll back the backtrack pointer and return nullptr
            
            template<template<typename>class PtrType>
            auto parse(const std::basic_string<KeyType> &keys ) -> Expression<DataType,PtrType>;
        private:
            auto call_skip(ParserInfo<KeyType, DataType>& info)->bool;
            auto call_constant_parser(ParserInfo<KeyType, DataType>& info)->bool;

            template<std::size_t I,std::size_t II>
            auto parse_name(ParserInfo<KeyType, DataType>& info) -> void;
            template<std::size_t I>
            auto insert(ParserInfo<KeyType, DataType>& info,NodeType* target) -> typename std::enable_if<I == variable_pos>::type;
            template<std::size_t I>
            auto insert(ParserInfo<KeyType, DataType>& info,NodeType* target) -> typename std::enable_if<I != variable_pos>::type;

            static auto insert_operator(ParserInfo<KeyType, DataType>& info,std::shared_ptr<OperatorEx<KeyType, DataType>> op) -> void;
            static auto insert_constant(ParserInfo<KeyType, DataType>& info,std::unique_ptr<DataType>&& data) -> void;
            static auto insert_variable(ParserInfo<KeyType, DataType>& info,std::shared_ptr<DataType> var) -> void;

            auto flush_operator_stack(ParserInfo<KeyType, DataType> &info) -> void;
        };

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        Node<MapType, KeyType, DataType...>::Node(const Node &other) : data(other.data), child(other.child)
        {
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        auto Node<MapType, KeyType, DataType...>::operator=(const Node &other) -> Node &
        {
            if (this != &other)
            {
                data = other.data;
                child = other.child;
            }
            return *this;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        Node<MapType, KeyType, DataType...>::Node(Node &&other) noexcept
            : data(std::move(other.data)), child(std::move(other.child))
        {
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        auto Node<MapType, KeyType, DataType...>::operator=(Node &&other) noexcept -> Node &
        {
            if (this != &other)
            {
                data = std::move(other.data);
                child = std::move(other.child);
            }
            return *this;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        template <std::size_t I>
        auto Node<MapType, KeyType, DataType...>::deep_copy_data_recursive(DataTuple &result) const -> void
        {
            using ElementType = typename std::tuple_element<I, DataTuple>::type::element_type;

            if (std::get<I>(data))
                std::get<I>(result) = std::make_shared<ElementType>(*std::get<I>(data));

            constexpr std::size_t Next = I + 1;
            if (Next < sizeof...(DataType))
                deep_copy_data_recursive<Next>(result);
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        auto Node<MapType, KeyType, DataType...>::deep_copy() const -> Node
        {
            Node result;

            if (sizeof...(DataType) > 0)
                deep_copy_data_recursive(result.data);

            for (const auto &pair : child)
                result.child[pair.first] = pair.second.deep_copy();

            return result;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        template <std::size_t I>
        auto Node<MapType, KeyType, DataType...>::get_data()
            -> typename std::tuple_element<I, DataTuple>::type
        {
            return std::get<I>(data);
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        template <std::size_t I>
        auto Node<MapType, KeyType, DataType...>::get_data() const
            -> std::shared_ptr<const typename std::tuple_element<I, DataTuple>::type::element_type>
        {
            return std::get<I>(data);
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        template <std::size_t I>
        auto Node<MapType, KeyType, DataType...>::set_data(
            typename std::tuple_element<I, DataTuple>::type value) -> void
        {
            std::get<I>(data) = std::move(value);
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        template <std::size_t I>
        auto Node<MapType, KeyType, DataType...>::remove_data() -> void
        {
            std::get<I>(data).reset();
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        template <std::size_t I>
        auto Node<MapType, KeyType, DataType...>::has_data() const -> bool
        {
            return std::get<I>(data) != nullptr;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        auto Node<MapType, KeyType, DataType...>::get_child() -> MapType<KeyType, Node> &
        {
            return child;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        auto Node<MapType, KeyType, DataType...>::get_child() const -> const MapType<KeyType, Node> &
        {
            return child;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        auto Node<MapType, KeyType, DataType...>::next(const KeyType &key) -> Node *
        {
            auto it = child.find(key);
            if (it != child.end())
                return &(it->second);
            return nullptr;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        auto Node<MapType, KeyType, DataType...>::next(const KeyType &key) const -> const Node *
        {
            auto it = child.find(key);
            if (it != child.end())
                return &(it->second);
            return nullptr;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        template <typename... Keys>
        auto Node<MapType, KeyType, DataType...>::search(const Keys &...keys) -> Node *
        {
            static_assert(sizeof...(Keys) > 0, "At least one key required");
            auto current(this);
            std::initializer_list<int>{(current = current ? current->next(keys) : current, 0)...};
            return current;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        template <typename... Keys>
        auto Node<MapType, KeyType, DataType...>::search(const Keys &...keys) const -> const Node *
        {
            static_assert(sizeof...(Keys) > 0, "At least one key required");
            const auto current(this);
            std::initializer_list<int>{(current = current ? current->next(keys) : current, 0)...};
            return current;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        template <typename Container>
        auto Node<MapType, KeyType, DataType...>::search(const Container &keys) -> decltype(std::begin(keys), std::end(keys), static_cast<Node*>(nullptr))
        {
            auto current(this);
            for (const auto &key : keys)
            {
                current = current->next(key);
                if (!current)
                    return nullptr;
            }
            return current;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        template <typename Container>
        auto Node<MapType, KeyType, DataType...>::search(const Container &keys) const -> decltype(std::begin(keys), std::end(keys), static_cast<const Node*>(nullptr))
        {
            const auto current(this);
            for (const auto &key : keys)
            {
                current = current->next(key);
                if (!current)
                    return nullptr;
            }
            return current;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        template <typename... Keys>
        auto Node<MapType, KeyType, DataType...>::insert(const Keys &...keys) -> Node *
        {
            static_assert(sizeof...(Keys) > 0, "At least one key required");
            auto current(this);
            std::initializer_list<int>{(current = &(current->get_child()[keys]), 0)...};
            return current;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        template <typename Container>
        auto Node<MapType, KeyType, DataType...>::insert(const Container &keys) -> decltype(std::begin(keys), std::end(keys), static_cast<Node*>(nullptr))
        {
            auto current(this);
            for (const auto &key : keys)
                current = &(current->get_child()[key]);
            return current;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        template <std::size_t J>
        auto Node<MapType, KeyType, DataType...>::has_any_data_impl() const ->
            typename std::enable_if<J == sizeof...(DataType), bool>::type
        {
            return false;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
            template <std::size_t J>
            auto Node<MapType, KeyType, DataType...>::has_any_data_impl() const ->
            typename std::enable_if < J<sizeof...(DataType), bool>::type
        {
            return has_data<J>() || has_any_data_impl<J + 1>();
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        auto Node<MapType, KeyType, DataType...>::has_any_data() const -> bool
        {
            return has_any_data_impl<0>();
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        template <std::size_t I, typename... Keys>
        auto Node<MapType, KeyType, DataType...>::remove(const Keys &...keys) -> bool
        {
            static_assert(sizeof...(Keys) > 0, "At least one key required");
            static_assert(I < sizeof...(DataType), "Data index out of range");

            const size_t key_count = sizeof...(Keys);
            std::vector<std::pair<Node *, const KeyType *>> path;
            path.reserve(key_count);

            auto current = this;
            const KeyType *key_array[] = {&keys...};

            for (size_t i = 0; i < key_count; ++i)
            {
                auto it = current->child.find(*key_array[i]);
                if (it == current->child.end())
                    return false;
                path.push_back({&(it->second), key_array[i]});
                current = &(it->second);
            }

            auto target = path.back().first;
            target->template remove_data<I>();

            for (auto it = path.rbegin(); it != path.rend(); ++it)
            {
                auto node = it->first;
                auto key = it->second;
                if (!node->has_any_data() && node->child.empty())
                {
                    auto parent = (it + 1 != path.rend()) ? (it + 1)->first : this;
                    parent->child.erase(*key);
                }
                else
                    break;
            }
            return true;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        template <std::size_t I, typename Container>
        auto Node<MapType, KeyType, DataType...>::remove(const Container &keys)
            -> decltype(std::begin(keys), std::end(keys), keys.size(), true)
        {
            static_assert(I < sizeof...(DataType), "Data index out of range");

            const size_t key_count = keys.size();
            if (key_count == 0)
                return false;

            std::vector<std::pair<Node *, const KeyType *>> path;
            path.reserve(key_count);

            auto current = this;
            auto it = std::begin(keys);
            auto end = std::end(keys);

            for (; it != end; ++it)
            {
                auto child_it = current->child.find(*it);
                if (child_it == current->child.end())
                    return false;
                path.push_back({&(child_it->second), &(*it)});
                current = &(child_it->second);
            }

            auto target = path.back().first;
            target->template remove_data<I>();

            for (auto rit = path.rbegin(); rit != path.rend(); ++rit)
            {
                auto node = rit->first;
                auto key = rit->second;
                if (!node->has_any_data() && node->child.empty())
                {
                    auto parent = (rit + 1 != path.rend()) ? (rit + 1)->first : this;
                    parent->child.erase(*key);
                }
                else
                    break;
            }
            return true;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename... DataType>
        auto Node<MapType, KeyType, DataType...>::clear_children() -> void
        {
            child.clear();
        }

        template <typename DataType, template <typename> class PtrType>
        template <typename U>
        auto Expression<DataType, PtrType>::value() const ->
            typename std::enable_if<!is_weak_ptr<U>::value, DataType>::type
        {
            std::list<std::unique_ptr<DataType>> cache;
            std:std::vector<DataType*> stack;
            auto operator_ptr = operators.begin();
            auto variable_ptr = variables.begin();
            auto constant_ptr = constants.begin();
            for(auto token:index)
                switch (token)
                {
                case TokenType::Constant:
                    if(constant_ptr==constants.end())
                        throw std::out_of_range("Constant iterator out of range");
                    stack.emplace_back(const_cast<DataType*>(constant_ptr->get()));
                    constant_ptr++;
                    break;
                case TokenType::Variale:
                    if(variable_ptr==variables.end())
                        throw std::out_of_range("Variable iterator out of range");
                    stack.emplace_back(const_cast<DataType*>(variable_ptr->get()));
                    variable_ptr++;
                    break;
                case TokenType::Operator:
                    if(operator_ptr==operators.end())
                        throw std::out_of_range("Operator iterator out of range");
                    if ((*operator_ptr).second > stack.size())
                        throw std::out_of_range("Operator require-size out of range");
                    if (!((*operator_ptr).first->function))
                        throw std::runtime_error("Wrong Operator");
                    cache.emplace_back(make_unique<DataType>((*(*operator_ptr).first).function(ParamViewer<DataType>((const_cast<DataType**>(&*stack.end())-(*operator_ptr).second),(*operator_ptr).second))));
                    stack.resize(stack.size()-(*operator_ptr).second);
                    stack.emplace_back(cache.back().get());
                    operator_ptr++;
                    break;
                }
            if (stack.size() != 1)
                throw std::logic_error("Expression evaluation failed: stack size not 1");
            return DataType(std::move(*stack.back()));
        }

        template <typename DataType, template <typename> class PtrType>
        template <typename U>
        auto Expression<DataType, PtrType>::value() const  ->
            typename std::enable_if<is_weak_ptr<U>::value, DataType>::type
        {
            Expression<DataType, std::shared_ptr> cache;

            cache.index = std::move(index);
            cache.constants = std::move(constants);

            for (const auto &weak_op : operators)
                if (auto shared_op = weak_op.lock())
                    cache.operators.push_back(shared_op);
                else
                    throw std::runtime_error("Weak pointer expired in operators");

            for (const auto &weak_var : variables)
                if (auto shared_var = weak_var.lock())
                    cache.variables.push_back(shared_var);
                else
                    throw std::runtime_error("Weak pointer expired in variables");

            auto result = cache.value();

            index = std::move(cache.index);
            constants = std::move(cache.constants);

            return result;
        }

        template <typename DataType, template <typename> class PtrType>
        template <
            template <typename> class OtherPtrType,
            typename std::enable_if<std::is_same<PtrType<DataType>, std::weak_ptr<DataType>>::value &&
                                    std::is_same<OtherPtrType<DataType>, std::shared_ptr<DataType>>::value>::type *>
        Expression<DataType, PtrType>::Expression(Expression<DataType, OtherPtrType> &&other) noexcept
            : index(std::move(other.index)), operators(convert_operators_shared_to_weak(std::move(other.operators))),
              variables(convert_variables_shared_to_weak(std::move(other.variables))),
              constants(std::move(other.constants))
        {
        }

        template <typename DataType, template <typename> class PtrType>
        template <template <typename> class OtherPtrType,
                  typename std::enable_if<std::is_same<PtrType<DataType>, std::shared_ptr<DataType>>::value &&
                                          std::is_same<OtherPtrType<DataType>, std::weak_ptr<DataType>>::value>::type *>
        Expression<DataType, PtrType>::Expression(Expression<DataType, OtherPtrType> &&other)
            : index(std::move(other.index)), operators(convert_operators_weak_to_shared(std::move(other.operators))),
              variables(convert_variables_weak_to_shared(std::move(other.variables))),
              constants(std::move(other.constants))
        {
        }

        template <typename DataType, template <typename> class PtrType>
        template <
            template <typename> class OtherPtrType,
            typename std::enable_if<std::is_same<PtrType<DataType>, std::weak_ptr<DataType>>::value &&
                                    std::is_same<OtherPtrType<DataType>, std::shared_ptr<DataType>>::value>::type *>
        auto Expression<DataType, PtrType>::operator=(Expression<DataType, OtherPtrType> &&other) noexcept
            -> Expression &
        {
            if (static_cast<const void *>(this) != static_cast<const void *>(&other))
            {
                index = std::move(other.index);
                operators = convert_operators_shared_to_weak(std::move(other.operators));
                variables = convert_variables_shared_to_weak(std::move(other.variables));
                constants = std::move(other.constants);
            }
            return *this;
        }

        template <typename DataType, template <typename> class PtrType>
        template <template <typename> class OtherPtrType,
                  typename std::enable_if<std::is_same<PtrType<DataType>, std::shared_ptr<DataType>>::value &&
                                          std::is_same<OtherPtrType<DataType>, std::weak_ptr<DataType>>::value>::type *>
        auto Expression<DataType, PtrType>::operator=(Expression<DataType, OtherPtrType> &&other) -> Expression &
        {
            if (static_cast<const void *>(this) != static_cast<const void *>(&other))
            {
                index = std::move(other.index);
                operators = convert_operators_weak_to_shared(std::move(other.operators));
                variables = convert_variables_weak_to_shared(std::move(other.variables));
                constants = std::move(other.constants);
            }
            return *this;
        }

        template <typename DataType, template <typename> class PtrType>
        auto Expression<DataType, PtrType>::convert_operators_shared_to_weak(
            std::list<std::pair<std::shared_ptr<Operator<DataType>>, std::size_t>> &&other_ops)
            -> std::list<std::pair<std::weak_ptr<Operator<DataType>>, std::size_t>>
        {
            std::list<std::pair<std::weak_ptr<Operator<DataType>>, std::size_t>> result;
            for (auto &op_pair : other_ops)
                result.emplace_back(std::weak_ptr<Operator<DataType>>(op_pair.first), op_pair.second);
            return result;
        }

        template <typename DataType, template <typename> class PtrType>
        auto Expression<DataType, PtrType>::convert_operators_weak_to_shared(
            std::list<std::pair<std::weak_ptr<Operator<DataType>>, std::size_t>> &&other_ops)
            -> std::list<std::pair<std::shared_ptr<Operator<DataType>>, std::size_t>>
        {
            std::list<std::pair<std::shared_ptr<Operator<DataType>>, std::size_t>> result;
            for (auto &op_pair : other_ops)
            {
                auto shared = op_pair.first.lock();
                if (!shared)
                    throw std::runtime_error("Expired weak_ptr");
                result.emplace_back(std::move(shared), op_pair.second);
            }
            return result;
        }

        template <typename DataType, template <typename> class PtrType>
        auto Expression<DataType, PtrType>::convert_variables_shared_to_weak(
            std::list<std::shared_ptr<DataType>> &&other_vars) -> std::list<std::weak_ptr<DataType>>
        {
            std::list<std::weak_ptr<DataType>> result;
            for (auto &var : other_vars)
                result.emplace_back(std::weak_ptr<DataType>(var));
            return result;
        }

        template <typename DataType, template <typename> class PtrType>
        auto
        Expression<DataType, PtrType>::convert_variables_weak_to_shared(std::list<std::weak_ptr<DataType>> &&other_vars)
            -> std::list<std::shared_ptr<DataType>>
        {
            std::list<std::shared_ptr<DataType>> result;
            for (auto &var : other_vars)
            {
                auto shared = var.lock();
                if (!shared)
                    throw std::runtime_error("Expired weak_ptr");
                result.emplace_back(std::move(shared));
            }
            return result;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename DataType>
        template <template <typename> class PtrType>
        auto ParserContext<MapType, KeyType, DataType>::parse(const std::basic_string<KeyType> &keys)-> Expression<DataType, PtrType>
        {
            static_assert(std::is_same<PtrType<DataType>, std::shared_ptr<DataType>>::value ||
                              std::is_same<PtrType<DataType>, std::weak_ptr<DataType>>::value,
                          "PtrType must be std::shared_ptr or std::weak_ptr");
            
            ParserInfo<KeyType,DataType> info(keys);

            while(info.pos < keys.size())
            {
                if(call_skip(info))
                    break;

                if(info.value_class)
                {
                    if(call_constant_parser(info))
                        continue;
                    parse_name<prefix_pos, variable_pos>(info);
                }
                else
                    parse_name<infix_pos, suffix_pos>(info);
            }
            flush_operator_stack(info);
            return Expression<DataType, PtrType>(std::move(info.expression));
        }

        template <template <typename, typename> class MapType, typename KeyType, typename DataType>
        auto ParserContext<MapType, KeyType, DataType>::call_skip(ParserInfo<KeyType, DataType>& info) -> bool
        {
            return skip&&skip(info);
        }

        template <template <typename, typename> class MapType, typename KeyType, typename DataType>
        auto ParserContext<MapType, KeyType, DataType>::call_constant_parser(ParserInfo<KeyType, DataType>& info) -> bool
        {
            if (constant_parser)
            {
                auto constant = constant_parser(info);
                if (constant)
                {
                    insert_constant(info, std::move(constant));
                    return true;
                }
            }
            return false;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename DataType>
        auto ParserContext<MapType, KeyType, DataType>::insert_variable(ParserInfo<KeyType, DataType>& info,std::shared_ptr<DataType> var) -> void
        {
            info.expression.variables.emplace_back(var);
            info.expression.index.emplace_back(TokenType::Variale);
            info.value_class = false;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename DataType>
        auto ParserContext<MapType, KeyType, DataType>::insert_operator(ParserInfo<KeyType, DataType>& info,std::shared_ptr<OperatorEx<KeyType, DataType>> op)-> void
        {
            if (!op)
                throw std::runtime_error("Wrong operator");

            if(op->extra_front&&op->extra_front(info))
                return;

            while (!info.stack.empty())
            {
                auto &top = info.stack.back();
                if(top.first->extra_mid)
                {
                    auto flag(top.first->extra_mid(info,op));
                    if(flag==BreakType::BREAK)
                        break;
                    else if(flag==BreakType::RETURN)
                        return;
                    else if(flag==BreakType::CONTINUE)
                        continue;
                }
                if (top.first->precedence < op->precedence ||(op->assoc == Associativity::Right && top.first->precedence == op->precedence))
                    break;
                info.expression.index.emplace_back(TokenType::Operator);
                info.expression.operators.emplace_back(top);
                info.stack.pop_back();
            }
            info.stack.emplace_back(std::make_pair(op,op->default_param_size));
            if(op->extra_back)
                op->extra_back(info);
        }

        template <template <typename, typename> class MapType, typename KeyType, typename DataType>
        auto ParserContext<MapType, KeyType, DataType>::insert_constant(ParserInfo<KeyType, DataType>& info,std::unique_ptr<DataType>&& data) -> void
        {
            info.expression.index.emplace_back(TokenType::Constant);
            info.expression.constants.emplace_back(std::move(data));
            info.value_class = false;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename DataType>
        template <std::size_t I, std::size_t II>
        auto ParserContext<MapType, KeyType, DataType>::parse_name(ParserInfo<KeyType, DataType> &info) -> void
        {
            std::size_t last_pos;
            auto nex(resource.next(info.keys[info.pos]));
            NodeType *last_one(nullptr);
            while (nex)
            {
                if (nex->template has_data<I>()||nex->template has_data<II>())
                {
                    last_one = nex;
                    last_pos = info.pos;
                }
                nex = nex->next(info.keys[++info.pos]);
                
            }

            if (!last_one)
                throw std::runtime_error("No valid operator or variable found in expression path");

            if (last_one->template has_data<I>())
                insert<I>(info,last_one);
            else
                insert<II>(info,last_one);

            info.pos = last_pos + 1;
        }

        template <template <typename, typename> class MapType, typename KeyType, typename DataType>
        template <std::size_t I>
        auto ParserContext<MapType, KeyType, DataType>::insert(ParserInfo<KeyType, DataType> &info,NodeType* target) -> typename std::enable_if<I == variable_pos>::type
        {
            insert_variable(info, target->template get_data<I>());
        }

        template <template <typename, typename> class MapType, typename KeyType, typename DataType>
        template <std::size_t I>
        auto ParserContext<MapType, KeyType, DataType>::insert(ParserInfo<KeyType, DataType> &info,NodeType* target) -> typename std::enable_if<I != variable_pos>::type
        {
            insert_operator(info, target->template get_data<I>());
        }

        template <template <typename, typename> class MapType, typename KeyType, typename DataType>
        auto ParserContext<MapType, KeyType, DataType>::flush_operator_stack(ParserInfo<KeyType, DataType> &info)
            -> void

        {
            while (!info.stack.empty())
            {
                auto &top = info.stack.back();
                if(top.first->extra_remaining)
                {
                    auto flag(top.first->extra_remaining(info));
                    if(flag==BreakType::BREAK)
                        break;
                    else if(flag==BreakType::RETURN)
                        return;
                    else if(flag==BreakType::CONTINUE)
                        continue;
                }
                info.expression.index.emplace_back(TokenType::Operator);
                info.expression.operators.emplace_back(top);
                info.stack.pop_back();
            }
        }
    }
}

#endif