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

#include <map>
#include <vector>
#include <future>
#include <string>
#include <memory>
#include <iostream>
#include <cstdlib>
#include <c4/range.hpp>
#include <c4/string.hpp>

namespace c4 {
    class cmd_opts {
        class cmd_flag {
            friend class cmd_opts;

            std::shared_ptr<bool> ptr;
            cmd_flag() : ptr(new bool) {
                *ptr = false;
            }

        public:
            operator bool() const {
                return *ptr;
            }
        };

        template<typename T>
        class cmd_opt {
            friend class cmd_opts;

            std::shared_ptr<std::string> ptr;

            cmd_opt() : ptr(std::make_shared<std::string>()) {}
            
            cmd_opt(const T& t) : ptr(std::make_shared<std::string>(to_string(t))) {}

        public:
            operator T() const {
                if (ptr->empty())
                    throw std::logic_error("Trying to get cmd_opt before parsing args");

                return string_to<T>(*ptr);
            }

            friend std::ostream& operator<<(std::ostream& out, const cmd_opt<T>& t) {
                return out << (T)t;
            }
        };

        class cmd_multi_opt {
            friend class cmd_opts;

            std::shared_ptr<std::vector<std::string>> ptr;

            cmd_multi_opt() : ptr(std::make_shared<std::vector<std::string>>()) {}
        public:
            operator std::vector<std::string>() const {
                return *ptr;
            }

            friend std::ostream& operator<<(std::ostream& out, const cmd_multi_opt& t) {
                out << "{";
				for (int i = 0; i < t.ptr->size(); i++) {
					if (i > 0)
						out << ", ";
					out << t.ptr->at(i);
				}
                return out << "}";
            }
        };

        std::map<std::string, std::shared_ptr<std::string>> optional;
        std::map<std::string, std::shared_ptr<std::string>> required;
        std::map<std::string, std::shared_ptr<std::vector<std::string>>> multiple;
        std::map<std::string, std::shared_ptr<bool>> flags;

        std::vector<std::pair<std::string, std::shared_ptr<std::string>>> required_free_args;

		std::map<std::string, std::string> explanations;

        std::vector<std::string> free_args_left;
        std::string arg0;

        std::string package;
        std::string version;
		std::string vendor;

        void assert_unique(const std::string name) const {
            if (optional.count(name) || required.count(name) || multiple.count(name) || flags.count(name))
                throw std::logic_error("Can't add multiple options with the same name: " + name);
        }

        void fail_with_error(const std::string& error) const {
            std::cout << error << std::endl;
			print_help();

            exit(EXIT_FAILURE);
        }

    public:

		cmd_opts() {
			add_flag("help", "Print this help message.");
        }

		void set_package(const std::string& package) {
			this->package = package;
		}

		void set_version(const std::string& version) {
			this->version = version;
		}

		void set_vendor(const std::string& vendor) {
			this->vendor = vendor;
		}

		void print_help() const {
			std::cout << "Usage: " << arg0;
			if (!explanations.empty()){
				std::cout << " [options]";
            }
			for (const auto& arg : required_free_args) {
				std::cout << " <" << arg.first << ">";
			}
            std::cout << std::endl;

			if (!explanations.empty()){
			    std::cout << "Options:" << std::endl;
			    for (const auto& opt : explanations) {
				    std::cout << "  " << opt.first << "\t" << opt.second << std::endl;
			    }
            }
		}

		void print_version() const {
			if (!package.empty()) {
			    std::cout << package << " version " << version;
			} else {
			    std::cout << "Version " << version;
			}

            if (!vendor.empty()) {
				std::cout << " by " << vendor;
            }
			std::cout << std::endl;
        }

        template<typename T, typename U>
        cmd_opt<T> add_optional(std::string name, const U& init, std::string explanation = "") {
			name = "--" + name;
            assert_unique(name);
            explanations[name] = explanation;
            cmd_opt<T> opt(init);
            optional.emplace(name, opt.ptr);
            return opt;
        }

        template<typename T>
        cmd_opt<T> add_required(std::string name, std::string explanation = "") {
			name = "--" + name;
            assert_unique(name);
            explanations[name] = explanation;
            cmd_opt<T> opt;
            required.emplace(name, opt.ptr);
            return opt;
        }

        template<typename T>
		cmd_opt<T> add_required_free_arg(std::string explanation) {
            cmd_opt<T> opt;
            required_free_args.emplace_back(explanation, opt.ptr);
            return opt;
        }

        cmd_multi_opt add_multiple(std::string name, std::string explanation = "") {
			name = "--" + name;
            assert_unique(name);
            explanations[name] = explanation;
            cmd_multi_opt opt;
            multiple.emplace(name, opt.ptr);
            return opt;
        }

        cmd_flag add_flag(std::string name, std::string explanation = "") {
			name = "--" + name;
            assert_unique(name);
            explanations[name] = explanation;
            cmd_flag flag;
            flags.emplace(name, flag.ptr);
            return flag;
        }

        void parse(const int argc, const char** argv) {
            if (!version.empty()) {
			    add_flag("version", "Print the version info of this application.");
            }

            if (argc == 2 && std::string(argv[1]) == "--help") {
				print_help();
                exit(EXIT_SUCCESS);
			}

			if (argc == 2 && std::string(argv[1]) == "--version" && !version.empty()) {
				print_version();
                exit(EXIT_SUCCESS);
			}

            arg0 = argv[0];
			int required_free_args_index = 0;
            for (int i = 1; i < argc;) {
                const std::string arg = argv[i++];

                auto optional_it = optional.find(arg);
                if (optional_it != optional.end()) {
                    if (i == argc)
                        fail_with_error("Cmd line option '" + arg + "' needs to have a following value");
                    *optional_it->second = argv[i++];
                    continue;
                }

                auto required_it = required.find(arg);
                if (required_it != required.end()) {
                    if (i == argc)
                        fail_with_error("Cmd line option '" + arg + "' needs to have a following value");
                    *required_it->second = argv[i++];
                    continue;
                }

                auto multiple_it = multiple.find(arg);
                if (multiple_it != multiple.end()) {
                    if (i == argc)
                        fail_with_error("Cmd line option '" + arg + "' needs to have a following value");
                    multiple_it->second->push_back(argv[i++]);
                    continue;
                }

                auto flag_it = flags.find(arg);
                if (flag_it != flags.end()) {
                    *flag_it->second = true;
                    continue;
                }

                if (required_free_args_index < required_free_args.size()) {
					*required_free_args[required_free_args_index].second = arg;
					required_free_args_index++;
					continue;
				}
                
                free_args_left.push_back(arg);
            }

            for (auto& opt : required) {
                if (opt.second->empty()) {
                    fail_with_error("Required cmd line argument not found: " + opt.first);
                }
            }

			if (required_free_args_index < required_free_args.size()) {
				fail_with_error(required_free_args[required_free_args_index].first + " not found in the command line");
			}
        }

        void parse(const int argc, char** argv) {
            parse(argc, const_cast<const char**>(argv));
        }
        
        const std::vector<std::string>& get_args_left() const {
            return free_args_left;
        }

        const std::string& get_arg0() const {
            return arg0;
        }
    };
};
