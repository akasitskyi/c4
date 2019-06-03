//MIT License
//
//Copyright(c) 2019 Alex Kasitskyi
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#pragma once

#include <array>
#include <tuple>
#include <memory>
#include <cstdint>
#include <type_traits>

namespace c4 {
    namespace serialize {
        using size_type = std::uint32_t;

        struct binary {
            void* ptr;
            size_type size;
        };

        template <typename Item>
        binary as_binary(Item* item, size_type count) {
            static_assert(std::is_trivially_copyable<Item>::value, "Must be trivially copyable");

            return { (void *)item, count * sizeof(Item) };
        }

        inline binary as_binary(void* data, size_type size) {
            return { data, size };
        }

        inline binary as_binary(const void* data, size_type size) {
            return { (void*)data, size };
        }

        template<class T>
        struct is_binary_serializable : std::integral_constant<bool, std::is_fundamental<T>::value || std::is_enum<T>::value> {};

        // std::vector - like
        template <typename Archive, typename Container,
            typename = decltype(std::declval<Container&>().size()),
            typename = decltype(std::declval<Container&>().begin()),
            typename = decltype(std::declval<Container&>().end()),
            typename = decltype(std::declval<Container&>().resize(std::size_t()))
        >
        std::enable_if_t<!is_binary_serializable<typename Container::value_type>::value> load(Archive& archive, Container& container) {
            size_type size = 0;

            archive(size);

            container.resize(size);

            for (auto& item : container) {
                archive(item);
            }
        };

        // std::vector - like
        template <typename Archive, typename Container,
            typename = decltype(std::declval<Container&>().size()),
            typename = decltype(std::declval<Container&>().begin()),
            typename = decltype(std::declval<Container&>().end()),
            typename = decltype(std::declval<Container&>().resize(std::size_t()))
        >
        std::enable_if_t<!is_binary_serializable<typename Container::value_type>::value> save(Archive& archive, const Container& container) {
            archive(static_cast<size_type>(container.size()));

            for (auto& item : container) {
                archive(item);
            }
        }

        // std::vector - like
        template <typename Archive, typename Container,
            typename = decltype(std::declval<Container&>().size()),
            typename = decltype(std::declval<Container&>().resize(std::size_t())),
            typename = decltype(std::declval<Container&>().data())
        >
        std::enable_if_t<is_binary_serializable<typename Container::value_type>::value> load(Archive& archive, Container& container) {
            size_type size = 0;

            archive(size);

            container.resize(size);

            if (!size) {
                return;
            }

            archive(as_binary(container.data(), static_cast<size_type>(container.size())));
        };

        // std::vector - like
        template <typename Archive, typename Container,
            typename = decltype(std::declval<Container&>().size()),
            typename = decltype(std::declval<Container&>().resize(std::size_t())),
            typename = decltype(std::declval<Container&>().data())
        >
        std::enable_if_t<is_binary_serializable<typename Container::value_type>::value> save(Archive& archive, const Container& container) {
            auto size = static_cast<size_type>(container.size());

            archive(size);

            if (!size) {
                return;
            }

            archive(as_binary(container.data(), static_cast<size_type>(container.size())));
        }

        // std::map - like
        template <typename Archive, typename Container,
            typename = decltype(std::declval<Container&>().size()),
            typename = decltype(std::declval<Container&>().begin()),
            typename = decltype(std::declval<Container&>().end()),
            typename = typename Container::mapped_type,
            typename = typename Container::key_type
        >
            void load(Archive& archive, Container& container) {
            size_type size = 0;

            archive(size);

            for (size_type i = 0; i < size; i++) {
                std::pair<typename Container::key_type, typename Container::mapped_type> item;

                archive(item);

                container.insert(std::move(item));
            }
        }

        // std::map - like
        template <typename Archive, typename Container,
            typename = decltype(std::declval<Container&>().size()),
            typename = decltype(std::declval<Container&>().begin()),
            typename = decltype(std::declval<Container&>().end()),
            typename = typename Container::mapped_type,
            typename = typename Container::key_type
        >
        void save(Archive& archive, const Container& container) {
            archive(static_cast<size_type>(container.size()));

            for (auto& item : container) {
                archive(item);
            }
        }

        template <typename Archive, typename Item, std::size_t size>
        void load(Archive& archive, Item(&array)[size]) {
            for (auto& item : array) {
                archive(item);
            }
        }

        template <typename Archive, typename Item, std::size_t size>
        void save(Archive& archive, const Item(&array)[size]) {
            for (auto& item : array) {
                archive(item);
            }
        }

        template <typename Archive, typename Item, std::size_t size>
        void load(Archive& archive, std::array<Item, size>& array) {
            for (auto& item : array) {
                archive(item);
            }
        }

        template <typename Archive, typename Item, std::size_t size>
        void save(Archive& archive, const std::array<Item, size>& array) {
            for (auto& item : array) {
                archive(item);
            }
        }

        template <typename Archive, typename First, typename Second>
        void load(Archive& archive, std::pair<First, Second>& pair) {
            archive(pair.first, pair.second);
        }

        template <typename Archive, typename First, typename Second>
        void save(Archive& archive, const std::pair<First, Second>& pair) {
            archive(pair.first, pair.second);
        }

        template <typename Archive, typename... TupleItems>
        void load(Archive& archive, std::tuple<TupleItems...>& tuple) {
            load(archive, tuple, std::make_index_sequence<sizeof...(TupleItems)>());
        }

        template <typename Archive, typename... TupleItems>
        void save(Archive& archive, const std::tuple<TupleItems...>& tuple) {
            save(archive, tuple, std::make_index_sequence<sizeof...(TupleItems)>());
        }

        template <typename Archive, typename... TupleItems, std::size_t... Indices>
        void load(Archive& archive, std::tuple<TupleItems...>& tuple, std::index_sequence<Indices...>) {
            archive(std::get<Indices>(tuple)...);
        }

        template <typename Archive, typename... TupleItems, std::size_t... Indices>
        void save(Archive& archive, const std::tuple<TupleItems...>& tuple, std::index_sequence<Indices...>) {
            archive(std::get<Indices>(tuple)...);
        }

        template <typename Archive, typename Type>
        void load(Archive& archive, std::unique_ptr<Type>& object) {
            auto loaded_object = std::unique_ptr<Type>(new Type());

            archive(*loaded_object);

            object.reset(loaded_object.release());
        }

        template <typename Archive, typename Type>
        void save(Archive& archive, const std::unique_ptr<Type>& object) {
            archive(*object);
        }

        template <typename Archive, typename Type>
        void load(Archive& archive, std::shared_ptr<Type>& object) {
            auto loaded_object = std::unique_ptr<Type>(new Type());

            archive(*loaded_object);

            object.reset(loaded_object.release());
        }

        template <typename Archive, typename Type>
        void save(Archive& archive, const std::shared_ptr<Type>& object) {
            archive(*object);
        }

        class output_archive {
            std::ostream& os;
        public:
            explicit output_archive(std::ostream& os) : os(os) {}

            template <typename Item, typename... Items>
            void operator()(Item&& first, Items&& ... items) {
                save_item(std::forward<Item>(first));

                operator()(std::forward<Items>(items)...);
            }

            void operator()() {}
        private:
            template <typename Item,
                typename = decltype(std::declval<Item&>().serialize(std::declval<output_archive&>()))
            >
            void save_item(Item&& item) {
                item.serialize(*this);
            }

            template <typename Item,
                typename = decltype(std::declval<Item&>().save(std::declval<output_archive&>())),
                typename = void
            >
            void save_item(Item&& item) {
                item.save(*this);
            }

            template <typename Item,
                typename = decltype(serialize(std::declval<output_archive&>(), std::declval<Item&>())),
                typename = void,
                typename = void
            >
            void save_item(Item&& item) {
                serialize(*this, item);
            }

            template <typename Item,
                typename = decltype(save(std::declval<output_archive&>(), std::declval<Item&>())),
                typename = void,
                typename = void,
                typename = void
            >
            void save_item(Item&& item) {
                save(*this, item);
            }

            template <typename Item>
            std::enable_if_t<is_binary_serializable<std::remove_reference_t<Item>>::value> save_item(Item&& item) {
                os.write((const char *)std::addressof(item), sizeof(item));
            }

            void save_item(binary&& item) {
                os.write((const char *)item.ptr, item.size);
            }
        };

        class input_archive {
            std::istream& is;
        public:
            explicit input_archive(std::istream& is) : is(is) {
                ASSERT_TRUE(is.good());
            }

            template <typename Item, typename... Items>
            void operator()(Item&& first, Items&& ... items) {
                load_item(std::forward<Item>(first));

                operator()(std::forward<Items>(items)...);
            }

            void operator()() {}
        private:
            template <typename Item,
                typename = decltype(std::declval<Item&>().serialize(std::declval<input_archive&>()))
            >
            void load_item(Item&& item) {
                item.serialize(*this);
            }

            template <typename Item,
                typename = decltype(std::declval<Item&>().load(std::declval<input_archive&>())),
                typename = void
            >
            void load_item(Item&& item) {
                item.load(*this);
            }

            template <typename Item,
                typename = decltype(serialize(std::declval<input_archive&>(), std::declval<Item&>())),
                typename = void,
                typename = void
            >
            void load_item(Item&& item) {
                serialize(*this, item);
            }

            template <typename Item,
                typename = decltype(load(std::declval<input_archive&>(), std::declval<Item&>())),
                typename = void,
                typename = void,
                typename = void
            >
            void load_item(Item&& item) {
                load(*this, item);
            }

            template <typename Item>
            std::enable_if_t<is_binary_serializable<std::remove_reference_t<Item>>::value> load_item(Item&& item) {
                is.read((char *)std::addressof(item), sizeof(item));
                ASSERT_TRUE(is.good());
            }

            void load_item(binary&& item) {
                is.read((char *)item.ptr, item.size);
                ASSERT_TRUE(is.good());
            }
        };

    } // namespace serialize
} // namespace c4
