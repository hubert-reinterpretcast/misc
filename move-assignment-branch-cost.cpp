
#include <chrono>
#include <iostream>
#include <vector>

namespace not_noop {
    class A
    {
        int* data_;
    public:
        ~A() { delete data_; }
        A(A&& a) noexcept
            : data_{ a.data_ }
        {
            a.data_ = nullptr;
        }

        A& operator=(A&& a) noexcept
        {
            delete data_;
            data_ = nullptr;
            data_ = a.data_;
            a.data_ = nullptr;
            return *this;
        }

        A(int i)
            : data_{ new int{i} }
        {
        }
    };
}

namespace noop {
    class A
    {
        int* data_;
    public:
        ~A() { delete data_; }
        A(A&& a) noexcept
            : data_{ a.data_ }
        {
            a.data_ = nullptr;
        }

        A& operator=(A&& a) noexcept
        {
            if (this != &a)
            {
                delete data_;
                data_ = a.data_;
                a.data_ = nullptr;
            }
            return *this;
        }

        A(int i)
            : data_{ new int{i} }
        {
        }
    };
}

namespace weird {
    class A
    {
        int* data_;
    public:
        ~A() { delete data_; }
        A(A&& a) noexcept
            : data_{ a.data_ }
        {
            a.data_ = nullptr;
        }

        A& operator=(A&& a) noexcept
        {
            if ((size_t)this & 0x0100)
            {
                delete data_;
                data_ = a.data_;
                a.data_ = nullptr;
            }
            return *this;
        }

        A(int i)
            : data_{ new int{i} }
        {
        }
    };
}

namespace nomove {
    class A
    {
        int* data_;
    public:
        ~A() { delete data_; }
        A(const A& a) 
            : data_{ new int{*a.data_} }
        {
        }

        A& operator=(const A& a)
        {
            if (this != &a)
            {
                delete data_;
                data_ = new int(*a.data_);
            }
            return *this;
        }

        A(int i)
            : data_{ new int{i} }
        {
        }
    };
}

int count[4] = { 0, 0, 0, 0 }, total[4] = { 0, 0, 0, 0 };

template <typename Callable>
auto opaque_call [[gnu::noipa]](const Callable &callable)
{
    return callable();
}

int
main()
{
    using namespace std;
    using namespace std::chrono;

    constexpr unsigned K = 5, N = 2'000'000;

    for (int i = 0; i < K; ++i) {
#if QA
        opaque_call([]
        {
            using namespace not_noop;
            vector<A> v;
            for (auto i = 0u; i < N; ++i)
                v.push_back(i);
            auto td = opaque_call([&v]
            {
                auto t0 = steady_clock::now();
                opaque_call([&v] { v.erase(v.begin()); });
                auto t1 = steady_clock::now();
                return t1 - t0;
            });
            count[0] = td.count() / 1000;
            cout << "\nnot_noop\t" << count[0];
            total[0] += count[0];
        });
#endif

#if QB
        opaque_call([]
        {
            using namespace noop;
            vector<A> v;
            for (auto i = 0u; i < N; ++i)
                v.push_back(i);
            auto td = opaque_call([&v]
            {
                auto t0 = steady_clock::now();
                opaque_call([&v] { v.erase(v.begin()); });
                auto t1 = steady_clock::now();
                return t1 - t0;
            });
            count[1] = int(td.count() / 1000);
            cout << "\nnoop    \t" << count[1] << "\t" << (1. * count[1] / count[0]);
            total[1] += count[1];
        });
#endif

#if QC
        opaque_call([]
        {
            using namespace weird;
            vector<A> v;
            for (auto i = 0u; i < N; ++i)
                v.push_back(i);
            auto td = opaque_call([&v]
            {
                auto t0 = steady_clock::now();
                opaque_call([&v] { v.erase(v.begin()); });
                auto t1 = steady_clock::now();
                return t1 - t0;
            });
            count[2] = int(td.count() / 1000);
            cout << "\nweird   \t" << count[2] << "\t" << (1. * count[2] / count[0]);
            total[2] += count[2];
        });
#endif

#if QD
        opaque_call([]
        {
            using namespace nomove;
            vector<A> v;
            for (auto i = 0u; i < N; ++i)
                v.push_back(i);
            auto td = opaque_call([&v]
            {
                auto t0 = steady_clock::now();
                opaque_call([&v] { v.erase(v.begin()); });
                auto t1 = steady_clock::now();
                return t1 - t0;
            });
            count[3] = int(td.count() / 1000);
            cout << "\nnomove  \t" << count[3] << "\t" << (1.*count[3]/count[0]) << "\n";
            total[3] += count[3];
        });
#endif
    }

    cout << "\n--- averages:";
    cout << "\nnot_noop\t" << total[0];
    cout << "\nnoop    \t" << total[1] << "\t" << (1.*total[1]/total[0]);
    cout << "\nweird   \t" << total[2] << "\t" << (1.*total[2]/total[0]);
    cout << "\nnomove  \t" << total[3] << "\t" << (1.*total[3]/total[0]) << "\n";
}
