#include <fmt/format.h>

#include <boost/pfr.hpp>
#include <boost/pfr/core_name.hpp>
#include <iostream>
#include <type_traits>
#include <nlohmann/json.hpp>
#include <fstream>
#include <string_view>

#define $fwd(X) std::forward<decltype(X)>(X)

namespace pfr_extension {
    template <std::size_t I_>
    struct FieldInfo {
        static constexpr auto I = I_;
        std::string_view name;
    };

    namespace details {
        template <std::size_t I>
        constexpr void one_field_consume(auto& t, auto& functor) {
            using T = std::remove_cvref_t<decltype(t)>;
            auto info = FieldInfo<I>{boost::pfr::get_name<I, T>()};
            functor(boost::pfr::get<I>(t), info);
        }
        template <std::size_t... I>
        constexpr void for_each_named_field_helper(
                auto&& t, auto&& functor, std::integer_sequence<std::size_t, I...>) {
            (one_field_consume<I>(t, functor), ...);
        }
    }  // namespace details

    constexpr void for_each_named_field(auto&& t, auto&& functor) {
        using T = std::remove_cvref_t<decltype(t)>;
        constexpr std::size_t N = boost::pfr::tuple_size_v<T>;
        auto seq = std::make_index_sequence<N>{};
        details::for_each_named_field_helper($fwd(t), $fwd(functor), seq);
    }
}  // namespace pfr_extension

#undef $fwd

namespace magic {
    template <typename T>
    concept InnerMagicable = requires { typename T::enable_magic; };

    template <typename T>
    constexpr bool extern_magicable = false;

    template <typename T>
    concept Magicable = InnerMagicable<T> || extern_magicable<T>;
}  // namespace magic

namespace fmt {

    template <magic::Magicable T>
    struct formatter<T, char> : formatter<std::string, char> {
        //{"sdsds": 1, ...}
        //{"sdsds": 2}
        static constexpr auto format(const T& t, auto& ctx) {
            auto ctx_out = ctx.out();
            ctx_out = format_to(ctx_out, "{{");
            auto one_field = [&](auto& field, auto info) {
                if constexpr (decltype(info)::I != 0) {
                    ctx_out = format_to(ctx_out, ",");
                }
                ctx_out = format_to(ctx_out, "\"{}\": {}", info.name, field);
            };
            pfr_extension::for_each_named_field(t, one_field);
            ctx_out = format_to(ctx_out, "}}");
            return ctx_out;
        }
    };

}  // namespace std


// partial specialization (full specialization works too)
namespace nlohmann {
    template <magic::Magicable T>
    struct adl_serializer<T> {
        static void to_json(json& j, const T& t) {
            auto one_field = [&](auto& field, auto info) {
                j[info.name] = field;
            };
            pfr_extension::for_each_named_field(t, one_field);
        }

        static void from_json(const json& j, T& t) {
            auto one_field = [&](auto& field, auto info) {
                j.at(info.name).get_to(field);
            };
            pfr_extension::for_each_named_field(t, one_field);
        }
    };
}

///////////////////////////library part off/////////////////////////////

struct User {
    std::string name;
    std::string password;
};

template <>
constexpr bool magic::extern_magicable<User> = true;

struct Data {
    User user;
    std::string advertise_data;
};

template <>
constexpr bool magic::extern_magicable<Data> = true;

namespace json {
    void dump(std::string_view file_name, auto& t){
        auto file = std::ofstream{file_name};
        file << nlohmann::json(t);
    }
    void load(std::string_view file_name, auto& t){
        auto file = std::ifstream{file_name};
        nlohmann::json j;
        file >> j;
        j.get_to(t);
    }
}

int main() {
    Data data;
    json::load("data.txt", data);
    fmt::print("{}\n", data);
    std::cout << "Введите новый логин: ";
    std::cin >> data.user.name;
    json::dump("data.txt", data);
}
