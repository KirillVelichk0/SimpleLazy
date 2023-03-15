#include <iostream>
#include <array>
#include <vector>
#include <functional>
#include <memory>
template <class T>
class LazyBase : public std::enable_shared_from_this<LazyBase<T>>
{
private:
    static void Deleter(LazyBase<T> *instanse)
    {
        if (instanse->isInited)
        {
            T *data = reinterpret_cast<T *>(instanse->data.data());
            std::cout << "deleted really\n";
            data->~T();
        }
        else{
            std::cout << "deleted pseudeo\n";
        }
    }
    std::vector<char> data;
    bool isInited;
    template <class... Types>
    std::function<void()> InitorGen(Types &&...initData)
    {
        constexpr bool isAllRval {(std::is_rvalue_reference_v<Types> && ...)};
        if constexpr(isAllRval){
            return [this, ... args = std::move<Types>(initData)]() mutable
        {
            this->data.resize(sizeof(T));
            new (data.data()) T(std::forward<Types>(args)...);
            this->isInited = true;
        };
        }
        return [this, ... args = initData]() mutable
        {
            this->data.resize(sizeof(T));
            new (data.data()) T(std::forward<Types>(args)...);
            this->isInited = true;
        };
    }
    std::function<void()> Initor;
    template <class... Types>
    LazyBase(Types &&...constructorArgs)
    {
        this->isInited = false;
        this->Initor = InitorGen(std::forward<Types>(constructorArgs)...);
    }
    LazyBase(const LazyBase<T> &data)
    {
        this->data = data.data;
        this->Initor = data.Initor;
        this->isInited = data.isInited;
    }

public:
    template <class... Types>
    static std::shared_ptr<LazyBase<T>> Construct(Types &&...initData)
    {
        std::shared_ptr<LazyBase<T>> result(new LazyBase<T>(std::forward<Types>(initData)...), &LazyBase<T>::Deleter);
        return result;
    }
    bool IsCreated() const noexcept{
        return this->isInited;
    }
    auto CreateCopy() const{
        std::shared_ptr<LazyBase<T>> result(new LazyBase<T>(*this), &LazyBase<T>::Deleter);
        return result;
    }
    auto GetInstanse()
    {
        return this->shared_from_this();
    }
    T &Get()
    {
        if (!this->isInited)
        {
            this->Initor();
            this->Initor = nullptr;
            this->isInited = true;
        }
        T *data = reinterpret_cast<T *>(this->data.data());
        return *data;
    }
};
// template <class T>
// using Lazy = std::shared_ptr<LazyBase<T>>;
int main(int, char **)
{
    {
        std::vector<int> ar(4, 3);
        auto data = LazyBase<std::vector<int>>::Construct(4, 3);
        std::cout << data.use_count() << '\n';
        auto anotherInst = data->GetInstanse();
        std::cout << data.use_count() << '\n';
        auto& cont = data->Get();
        for(auto val: cont){
            std::cout << val << " val ";
        }
        std::cout << '\n';
    }
    {
       auto data= LazyBase<int>::Construct(7);
       std::cout << data->Get() << '\n';
    }
    {
        auto data = LazyBase<int>::Construct();
        std::cout << data->Get() << '\n';
        auto dataCopy = data->CreateCopy();
        std::cout << data.use_count() << '\n';
        std::cout << data->Get() << '\n';
        std::cout << dataCopy.use_count() << '\n';
        std::cout << dataCopy->Get() << '\n';

    }
}
