#ifndef EVAL_HPP
#define EVAL_HPP

#include <map>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <stdexcept>
#include <limits>

namespace eval
{
    constexpr size_t size_max = std::numeric_limits<size_t>::max();

    template <typename CharType, typename DataType>
    class sstree
    {
    public:
        struct tree_in
        {
            std::map<CharType, tree_in> child;
            DataType *data;
            bool isdelete;
            tree_in() : data(nullptr),isdelete(false) {}
            ~tree_in() { if(isdelete)delete data; }
        };
        using iterator = tree_in *;

    private:
        tree_in tree;
        iterator ptr;

    public:
        sstree() : ptr(&tree) {}

        DataType *data() { return ptr->data; }
        iterator begin() { return &tree; }
        sstree &rebegin()
        {
            ptr = &tree;
            return *this;
        }
        sstree &operator()(iterator it)
        {
            ptr = it;
            return *this;
        }
        sstree &setptr(iterator it) { return operator()(it); }
        iterator getptr() { return ptr; }
        std::map<CharType, tree_in> &map() { return ptr->child; }

        std::pair<bool,iterator> insert(const std::basic_string<CharType> &str, const DataType &data);
        std::pair<bool,iterator> insert(const std::basic_string<CharType> &str, DataType *data);
        bool reset(const std::basic_string<CharType> &str, const DataType &data);
        bool reset(const std::basic_string<CharType> &str, DataType *data);

        iterator find(const CharType &ch)
        {
            auto ptr_r = ptr->child.find(ch);
            return ptr_r == ptr->child.end() ? nullptr : &(ptr_r->second);
        }

        iterator search(const std::basic_string<CharType> &str);

        bool erase(const std::basic_string<CharType> &str);
    };

    template <typename CharType, typename DataType>
    std::pair<bool,typename sstree<CharType,DataType>::iterator> sstree<CharType,DataType>::insert(const std::basic_string<CharType> &str, const DataType &data)
    {
        iterator ptr_l = &tree;
        typename std::map<CharType, tree_in>::iterator ptr_r;
        for (size_t pos = 0; pos < str.size(); pos++)
        {
            ptr_r = ptr_l->child.find(str[pos]);
            if (ptr_r != ptr_l->child.end())
            {
                ptr_l = &(ptr_r->second);
            }
            else
            {
                do
                {
                    ptr_l = &(ptr_l->child[str[pos]]);
                } while (++pos < str.size());
                break;
            }
        }
        if (ptr_l->data)
            return {false,ptr_l};
        ptr_l->data = new DataType(data);
        ptr_l->isdelete = true;
        return {true,ptr_l};
    }

    template <typename CharType, typename DataType>
    std::pair<bool,typename sstree<CharType,DataType>::iterator> sstree<CharType,DataType>::insert(const std::basic_string<CharType> &str, DataType *data)
    {
        iterator ptr_l = &tree;
        typename std::map<CharType, tree_in>::iterator ptr_r;
        for (size_t pos = 0; pos < str.size(); pos++)
        {
            ptr_r = ptr_l->child.find(str[pos]);
            if (ptr_r != ptr_l->child.end())
            {
                ptr_l = &(ptr_r->second);
            }
            else
            {
                do
                {
                    ptr_l = &(ptr_l->child[str[pos]]);
                } while (++pos < str.size());
                break;
            }
        }
        if (ptr_l->data)
            return {false,ptr_l};
        ptr_l->data = data;
        return {true,ptr_l};
    }

    template <typename CharType, typename DataType>
    bool sstree<CharType,DataType>::reset(const std::basic_string<CharType> &str, const DataType &data)
    {
        auto ptr_l = search(str);
        if(!ptr_l)
            return false;
        if(ptr_l->isdelete)
            delete ptr_l->data;
        else 
            ptr_l->isdelete=true;

        ptr_l->data=new DataType(data);

        return true;
    }

    template <typename CharType, typename DataType>
    bool sstree<CharType,DataType>::reset(const std::basic_string<CharType> &str, DataType *data)
    {
        auto ptr_l = search(str);
        if(!ptr_l)
            return false;
        
        if(ptr_l->isdelete)
        {
            delete ptr_l->data;
            ptr_l->isdelete=false;
        }

        ptr_l->data=data;

        return true;
    }

    template <typename CharType, typename DataType>
    typename sstree<CharType,DataType>::iterator sstree<CharType,DataType>::search(const std::basic_string<CharType> &str)
    {
        typename std::map<CharType, tree_in>::iterator ptr_r;
        iterator ptr_l = ptr;
        for (const CharType &ch : str)
        {
            ptr_r = ptr_l->child.find(ch);
            if (ptr_r == ptr_l->child.end())
                return nullptr;
            ptr_l = &(ptr_r->second);
        }
        return ptr_l;
    }
    template <typename CharType, typename DataType>
    bool sstree<CharType,DataType>::erase(const std::basic_string<CharType> &str)
    {
        iterator ptr_l = &tree;
        typename std::map<CharType, tree_in>::iterator ptr_r;
        size_t last_branch = 0;
        iterator last_node = &tree;
        for (size_t pos = 0; pos < str.size(); ++pos)
        {
            ptr_r = ptr_l->child.find(str[pos]);
            if (ptr_r == ptr_l->child.end())
                return false;
            if (ptr_l->child.size() > 1 || ptr_l->data)
            {
                last_node = ptr_l;
                last_branch = pos;
            }
            ptr_l = &(ptr_r->second);
        }
        if (!ptr_l->child.empty())
        {
            if(ptr_l->isdelete)
            {
                delete ptr_l->data;
                ptr_l->isdelete=false;
            }
            ptr_l->data = nullptr;
        }
        else
        {
            last_node->child.erase(str[last_branch]);
        }
        return true;
    }
    template <typename Type>
    struct func
    {
        size_t size;
        size_t priority;
        std::function<Type(const Type *)> func_ptr;
    };

    enum class vartype
    {
        CONSTVAR,
        FREEVAR
    };

    template <typename Type>
    struct epre
    {
        std::vector<func<Type> *> funcs;
        std::vector<Type *> vars;
        std::vector<Type> consts;
        std::string index;
        void clear()
        {
            funcs.clear();
            vars.clear();
            consts.clear();
            index.clear();
        }
    };

    template <typename CharType, typename DataType>
    struct evaluator
    {
        using StringType = std::basic_string<CharType>;

        std::function<bool(const StringType &,size_t&,epre<DataType> &)> consts;

        std::shared_ptr<sstree<CharType, DataType>> vars;
        std::shared_ptr<sstree<CharType, func<DataType>>> funcs;
        std::shared_ptr<sstree<CharType, func<DataType>>> prefix_ops;
        std::shared_ptr<sstree<CharType, func<DataType>>> infix_ops;
        std::shared_ptr<sstree<CharType, func<DataType>>> suffix_ops;

        evaluator(
            std::function<bool(const StringType &,size_t&,epre<DataType> &)> consts_,
            std::shared_ptr<sstree<CharType, DataType>> vars_ = nullptr,
            std::shared_ptr<sstree<CharType, func<DataType>>> funcs_ = nullptr,
            std::shared_ptr<sstree<CharType, func<DataType>>> pre_ops = nullptr,
            std::shared_ptr<sstree<CharType, func<DataType>>> in_ops = nullptr,
            std::shared_ptr<sstree<CharType, func<DataType>>> suf_ops = nullptr) : consts(consts_),
                                                                                   vars(vars_ ? vars_ : std::make_shared<sstree<CharType, DataType>>()),
                                                                                   funcs(funcs_ ? funcs_ : std::make_shared<sstree<CharType, func<DataType>>>()),
                                                                                   prefix_ops(pre_ops ? pre_ops : std::make_shared<sstree<CharType, func<DataType>>>()),
                                                                                   infix_ops(in_ops ? in_ops : std::make_shared<sstree<CharType, func<DataType>>>()),
                                                                                   suffix_ops(suf_ops ? suf_ops : std::make_shared<sstree<CharType, func<DataType>>>())
        {}
        epre<DataType> parse(const StringType &str);
        size_t parse(epre<DataType> &expr, const StringType &str) noexcept;
        DataType evaluate(const epre<DataType> &expr);
        typename sstree<CharType, func<DataType>>::iterator insert_function(const StringType&str,const std::vector<StringType>&vars,const StringType&context);
    };
    template <typename CharType, typename DataType>
    size_t evaluator<CharType,DataType>::parse(epre<DataType> &expr, const StringType &str) noexcept
    {
        std::vector<func<DataType> *> op_stack;
        size_t pos = 0;
        bool expecting_operand = true;

        while (pos < str.size())
        {
            if (isspace(str[pos]))
            {
                pos++;
                continue;
            }
            if (expecting_operand)
            {
                if (str[pos] == '(')
                {
                    op_stack.push_back(nullptr);
                    pos++;
                    continue;
                }
                if(consts(str,pos,expr))
                {
                    expecting_operand = false;
                    continue;
                }
                typename sstree<CharType, func<DataType>>::iterator it = prefix_ops->find(str[pos]);
                if (it)
                {
                    size_t start = pos;
                    prefix_ops->setptr(it);
                    pos++;
                    while (pos < str.size() && (it = prefix_ops->find(str[pos])))
                    {
                        prefix_ops->setptr(it);
                        pos++;
                    }

                    if (prefix_ops->data())
                    {
                        op_stack.push_back(prefix_ops->data());
                        prefix_ops->rebegin();
                        continue;
                    }
                    prefix_ops->rebegin();
                    pos = start; 
                }
                it = funcs->find(str[pos]);
                if (it)
                {
                    size_t start = pos;
                    funcs->setptr(it);
                    pos++;
                    while (pos < str.size() && (it = funcs->find(str[pos])))
                    {
                        funcs->setptr(it);
                        pos++;
                    }

                    if (pos < str.size() && str[pos] == '(' && funcs->data())
                    {
                        op_stack.push_back(funcs->data());
                        op_stack.push_back(nullptr); 
                        pos++;                       
                        expecting_operand = true;
                        funcs->rebegin();
                        continue;
                    }
                    funcs->rebegin();
                    pos = start; 
                }
                typename sstree<CharType, DataType>::iterator var_it = vars->find(str[pos]);
                if (var_it)
                {
                    size_t start = pos;
                    vars->setptr(var_it);
                    pos++;
                    while (pos < str.size() && (var_it = vars->find(str[pos])))
                    {
                        vars->setptr(var_it);
                        pos++;
                    }

                    if (vars->data())
                    {
                        expr.vars.push_back(vars->data());
                        expr.index += 'v';
                        expecting_operand = false;
                        vars->rebegin();
                        continue;
                    }
                    vars->rebegin();
                    pos = start; 
                }
            }
            if (!expecting_operand)
            {
                if (str[pos] == ')')
                {
                    while (!op_stack.empty() && op_stack.back() != nullptr)
                    {
                        expr.funcs.push_back(op_stack.back());
                        expr.index += 'f';
                        op_stack.pop_back();
                    }
                    if (op_stack.empty())
                        return pos;
                    op_stack.pop_back();
                    pos++;
                    continue;
                }
                if (str[pos] == ',')
                {
                    pos++;
                    expecting_operand = true;
                    continue;
                }
                typename sstree<CharType, func<DataType>>::iterator op_it = infix_ops->find(str[pos]);
                if (op_it)
                {
                    size_t start = pos;
                    infix_ops->setptr(op_it);
                    pos++;
                    while (pos < str.size() && (op_it = infix_ops->find(str[pos])))
                    {
                        infix_ops->setptr(op_it);
                        pos++;
                    }

                    if (infix_ops->data())
                    {
                        
                        while (!op_stack.empty() && op_stack.back() != nullptr &&
                               op_stack.back()->priority >= infix_ops->data()->priority)
                        {
                            expr.funcs.push_back(op_stack.back());
                            expr.index += 'f';
                            op_stack.pop_back();
                        }
                        op_stack.push_back(infix_ops->data());
                        expecting_operand = true;
                        infix_ops->rebegin();
                        continue;
                    }
                    infix_ops->rebegin();
                    pos = start; 
                }
                op_it = suffix_ops->find(str[pos]);
                if (op_it)
                {
                    size_t start = pos;
                    suffix_ops->setptr(op_it);
                    pos++;
                    while (pos < str.size() && (op_it = suffix_ops->find(str[pos])))
                    {
                        suffix_ops->setptr(op_it);
                        pos++;
                    }

                    if (suffix_ops->data())
                    {
                        
                        expr.funcs.push_back(suffix_ops->data());
                        expr.index += 'f';
                        suffix_ops->rebegin();
                        continue;
                    }
                    suffix_ops->rebegin();
                    pos = start; 
                }
            }
            return pos;
        }

        while (!op_stack.empty())
        {
            if (op_stack.back() == nullptr)
            {
                return str.size(); 
            }
            expr.funcs.push_back(op_stack.back());
            expr.index += 'f';
            op_stack.pop_back();
        }

        return size_max; 
    }
    template <typename CharType, typename DataType>
    epre<DataType> evaluator<CharType,DataType>::parse(const StringType &str)
    {
        epre<DataType> expr;
        std::vector<func<DataType> *> op_stack;
        size_t pos = 0;
        bool expecting_operand = true;

        while (pos < str.size())
        {
            if (isspace(str[pos]))
            {
                pos++;
                continue;
            }
            if (expecting_operand)
            {
                if (str[pos] == '(')
                {
                    op_stack.push_back(nullptr);
                    pos++;
                    continue;
                }
                if(consts(str,pos,expr))
                {
                    expecting_operand = false;
                    continue;
                }
                typename sstree<CharType, func<DataType>>::iterator it = prefix_ops->find(str[pos]);
                if (it)
                {
                    size_t start = pos;
                    prefix_ops->setptr(it);
                    pos++;
                    while (pos < str.size() && (it = prefix_ops->find(str[pos])))
                    {
                        prefix_ops->setptr(it);
                        pos++;
                    }

                    if (prefix_ops->data())
                    {
                        op_stack.push_back(prefix_ops->data());
                        prefix_ops->rebegin();
                        continue;
                    }
                    prefix_ops->rebegin();
                    pos = start; 
                }
                it = funcs->find(str[pos]);
                if (it)
                {
                    size_t start = pos;
                    funcs->setptr(it);
                    pos++;
                    while (pos < str.size() && (it = funcs->find(str[pos])))
                    {
                        funcs->setptr(it);
                        pos++;
                    }

                    if (pos < str.size() && str[pos] == '(' && funcs->data())
                    {
                        op_stack.push_back(funcs->data());
                        op_stack.push_back(nullptr); 
                        pos++;                       
                        expecting_operand = true;
                        funcs->rebegin();
                        continue;
                    }
                    funcs->rebegin();
                    pos = start; 
                }
                typename sstree<CharType, DataType>::iterator var_it = vars->find(str[pos]);
                if (var_it)
                {
                    size_t start = pos;
                    vars->setptr(var_it);
                    pos++;
                    while (pos < str.size() && (var_it = vars->find(str[pos])))
                    {
                        vars->setptr(var_it);
                        pos++;
                    }

                    if (vars->data())
                    {
                        expr.vars.push_back(vars->data());
                        expr.index += 'v';
                        expecting_operand = false;
                        vars->rebegin();
                        continue;
                    }
                    vars->rebegin();
                    pos = start; 
                }
            }
            if (!expecting_operand)
            {
                if (str[pos] == ')')
                {
                    while (!op_stack.empty() && op_stack.back() != nullptr)
                    {
                        expr.funcs.push_back(op_stack.back());
                        expr.index += 'f';
                        op_stack.pop_back();
                    }
                    if (op_stack.empty())
                        throw pos;
                    op_stack.pop_back();
                    pos++;
                    continue;
                }
                if (str[pos] == ',')
                {
                    while (!op_stack.empty() && op_stack.back() != nullptr)
                    {
                        expr.funcs.push_back(op_stack.back());
                        expr.index += 'f';
                        op_stack.pop_back();
                    }
                    if(op_stack.empty())
                        throw pos;
                    pos++;
                    expecting_operand = true;
                    continue;
                }
                typename sstree<CharType, func<DataType>>::iterator op_it = infix_ops->find(str[pos]);
                if (op_it)
                {
                    size_t start = pos;
                    infix_ops->setptr(op_it);
                    pos++;
                    while (pos < str.size() && (op_it = infix_ops->find(str[pos])))
                    {
                        infix_ops->setptr(op_it);
                        pos++;
                    }

                    if (infix_ops->data())
                    {
                        
                        while (!op_stack.empty() && op_stack.back() != nullptr &&
                               op_stack.back()->priority >= infix_ops->data()->priority)
                        {
                            expr.funcs.push_back(op_stack.back());
                            expr.index += 'f';
                            op_stack.pop_back();
                        }
                        op_stack.push_back(infix_ops->data());
                        expecting_operand = true;
                        infix_ops->rebegin();
                        continue;
                    }
                    infix_ops->rebegin();
                    pos = start; 
                }
                op_it = suffix_ops->find(str[pos]);
                if (op_it)
                {
                    size_t start = pos;
                    suffix_ops->setptr(op_it);
                    pos++;
                    while (pos < str.size() && (op_it = suffix_ops->find(str[pos])))
                    {
                        suffix_ops->setptr(op_it);
                        pos++;
                    }

                    if (suffix_ops->data())
                    {
                        
                        expr.funcs.push_back(suffix_ops->data());
                        expr.index += 'f';
                        suffix_ops->rebegin();
                        continue;
                    }
                    suffix_ops->rebegin();
                    pos = start; 
                }
            }
            throw pos;
        }

        while (!op_stack.empty())
        {
            if (op_stack.back() == nullptr)
            {
                throw str.size(); 
            }
            expr.funcs.push_back(op_stack.back());
            expr.index += 'f';
            op_stack.pop_back();
        }

        return expr; 
    }
    template <typename CharType, typename DataType>
    DataType evaluator<CharType,DataType>::evaluate(const epre<DataType> &expr)
    {
        std::vector<DataType> stack;
        size_t func_idx = 0, var_idx = 0, const_idx = 0;

        for (char ch : expr.index)
        {
            switch (ch)
            {
            case 'f':
            {
                auto &f = expr.funcs[func_idx++];
                if (stack.size() < f->size)
                    throw std::runtime_error("Stack underflow");
                auto temp=f->func_ptr(stack.data() + stack.size() - f->size);
                stack.resize(stack.size() - f->size+1);
                stack.back()=temp;
                break;
            }
            case 'v':
                stack.push_back(*expr.vars[var_idx++]);
                break;
            case 'c':
                stack.push_back(expr.consts[const_idx++]);
                break;
            default:
                throw std::runtime_error("Invalid expression index");
            }
        }
        if (stack.size() != 1)
            throw std::runtime_error("Malformed expression");
        return stack.back();
    }
    template <typename CharType, typename DataType>
    typename sstree<CharType, func<DataType>>::iterator evaluator<CharType,DataType>::insert_function(const StringType&str,const std::vector<StringType>&vars_temp,const StringType&context)
    {
        struct VarInsertResult
        {
            bool inserted;                                      
            typename sstree<CharType, DataType>::iterator node; 
            DataType *old_data;                                 
        };

        std::vector<VarInsertResult> var_results(vars_temp.size());
        auto vars_tmpv = std::make_shared<std::vector<DataType>>(vars_temp.size());

        for (size_t pos = 0; pos < vars_temp.size(); ++pos)
        {
            const auto &var_name = vars_temp[pos];
            auto &tmp_value = (*vars_tmpv)[pos];

            auto insert_result = vars->insert(var_name, &tmp_value);
            var_results[pos].inserted = insert_result.first;
            var_results[pos].node = insert_result.second;

            if (!insert_result.first)
            {
                var_results[pos].old_data = var_results[pos].node->data;
                var_results[pos].node->data = &tmp_value;
            }
        }

        epre<DataType> expr;
        bool sign=false;
        size_t pos;
        try
        {
            expr = this->parse(context);
        }
        catch (size_t tpos)
        {
            sign=true;
            pos=tpos;
            throw;
        }

        for (auto &res : var_results)
        {
            if (!res.inserted)
                res.node->data = res.old_data;
            else
                vars->erase(vars_temp[&res - &var_results[0]]);
        }

        if(sign)
            throw pos;

        std::unique_ptr<func<DataType>> f(new func<DataType>());
        f->size = vars_temp.size();
        f->priority = size_max; 

        f->func_ptr = [vars_tmpv, expr, this](const DataType *args) -> DataType
        {
            for (size_t i = 0; i < vars_tmpv->size(); ++i)
                (*vars_tmpv)[i] = args[i];
            return this->evaluate(expr);
        };

        auto insert_result = funcs->insert(str, f.get());
        if (!insert_result.first)
            throw std::runtime_error("Function already exists: " + std::string(str.begin(), str.end()));

        insert_result.second->isdelete = true;
        f.release();

        return insert_result.second;
    }
}

#endif