#!/usr/bin/env bash
set -euo pipefail

project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
source "$project_dir/deps/versions.env"
cache_dir="$project_dir/.deps"
simhash_dir="$cache_dir/simhash"
cppjieba_dir="$simhash_dir/submodules/cppjieba"
limonp_dir="$cppjieba_dir/deps/limonp"
prefix=${2:-/usr/local}
case "$prefix" in
    /*) ;;
    *) echo "PREFIX 必须是绝对路径" >&2; exit 1 ;;
esac
manifest="$prefix/share/search-engine/deps/versions.env"
dict_dir="$prefix/share/search-engine/dict"

fetch_repo() {
    local name=$1 url=$2 version=$3 commit=$4
    local repo="$cache_dir/$name"
    if [[ ! -d "$repo/.git" ]]; then
        git clone --depth 1 --branch "$version" "$url" "$repo"
    fi
    if [[ -n $(git -C "$repo" status --porcelain) ]]; then
        echo "$repo 中有本地修改，请先保存，再安装依赖。" >&2
        exit 1
    fi
    if [[ $(git -C "$repo" rev-parse HEAD) != "$commit" ]]; then
        git -C "$repo" fetch --depth 1 origin "$commit"
        git -C "$repo" checkout --detach "$commit"
    fi
    [[ $(git -C "$repo" rev-parse HEAD) == "$commit" ]]
}

fetch_deps() {
    mkdir -p "$cache_dir"
    fetch_repo simhash https://github.com/yanyiwu/simhash.git "$SIMHASH_VERSION" "$SIMHASH_COMMIT"
    # 整套获取 SimHash -> cppjieba -> limonp，沿用上游记录的子模块版本。
    git -C "$simhash_dir" submodule update --init --recursive --depth 1
    [[ -z $(git -C "$simhash_dir" status --porcelain) ]]
    fetch_repo json https://github.com/nlohmann/json.git "$JSON_VERSION" "$JSON_COMMIT"
    echo "依赖源码已就绪：SimHash $SIMHASH_VERSION（含 cppjieba、limonp）/ JSON $JSON_VERSION"
}

installed() {
    cmp -s "$project_dir/deps/versions.env" "$manifest" || return 1
    cmp -s "$project_dir/dict/user.dict.utf8" "$dict_dir/user.dict.utf8" || return 1
    cmp -s "$project_dir/dict/user.dict.utf8" "$prefix/dict/user.dict.utf8" || return 1
    local file
    for file in include/cppjieba/Jieba.hpp include/limonp/Logging.hpp \
        include/simhash/Simhasher.hpp include/nlohmann/json.hpp \
        share/search-engine/dict/jieba.dict.utf8 share/search-engine/dict/hmm_model.utf8 \
        share/search-engine/dict/idf.utf8 share/search-engine/dict/stop_words.utf8; do
        [[ -r "$prefix/$file" ]] || return 1
    done
}

install_tree() {
    local source_dir=$1 destination=$2 file
    while IFS= read -r -d '' file; do
        "${install_cmd[@]}" -D -m 644 "$file" "$destination/${file#"$source_dir/"}"
    done < <(find "$source_dir" -type f -print0)
}

install_deps() {
    if installed; then
        echo "依赖已安装且版本一致：$prefix"
        return
    fi
    fetch_deps
    local writable_parent=$prefix
    while [[ ! -e "$writable_parent" ]]; do
        writable_parent=$(dirname -- "$writable_parent")
    done
    local -a install_cmd=(install)
    if [[ ! -w "$writable_parent" || ( -d "$prefix/include" && ! -w "$prefix/include" ) ]]; then
        echo "向 $prefix 安装头文件和词典，需要 sudo 权限。"
        install_cmd=(sudo install)
    fi
    install_tree "$simhash_dir/include/simhash" "$prefix/include/simhash"
    install_tree "$cppjieba_dir/include/cppjieba" "$prefix/include/cppjieba"
    install_tree "$limonp_dir/include/limonp" "$prefix/include/limonp"
    install_tree "$cache_dir/json/single_include/nlohmann" "$prefix/include/nlohmann"
    local file
    for file in jieba.dict.utf8 hmm_model.utf8 idf.utf8 stop_words.utf8; do
        "${install_cmd[@]}" -D -m 644 "$cppjieba_dir/dict/$file" "$dict_dir/$file"
    done
    # 保留本项目的自定义词条。
    "${install_cmd[@]}" -D -m 644 "$project_dir/dict/user.dict.utf8" "$dict_dir/user.dict.utf8"
    # SimHash v1.3.3 固定传入空 user_dict_path；其自带的 cppjieba 会从
    # 根据 __FILE__ 推导出的 PREFIX/dict/user.dict.utf8 加载默认用户词典。
    "${install_cmd[@]}" -D -m 644 "$project_dir/dict/user.dict.utf8" "$prefix/dict/user.dict.utf8"
    "${install_cmd[@]}" -D -m 644 "$cppjieba_dir/LICENSE" "$prefix/share/doc/search-engine-deps/cppjieba-LICENSE"
    "${install_cmd[@]}" -D -m 644 "$limonp_dir/LICENSE" "$prefix/share/doc/search-engine-deps/limonp-LICENSE"
    "${install_cmd[@]}" -D -m 644 "$cache_dir/json/LICENSE.MIT" "$prefix/share/doc/search-engine-deps/json-LICENSE.MIT"
    # SimHash 的 Jenkins 实现将许可说明放在源文件内。
    "${install_cmd[@]}" -D -m 644 "$project_dir/deps/versions.env" "$manifest"
    echo "依赖安装完成：$prefix/include；词典：$dict_dir"
}

case "${1:-}" in
    fetch) fetch_deps ;;
    install) install_deps ;;
    check)
        if ! installed; then
            echo "依赖尚未安装或版本不一致，请运行 make build PREFIX=$prefix" >&2
            exit 1
        fi
        ;;
    *) echo "用法：bash scripts/deps.sh {fetch|install|check} [安装前缀]" >&2; exit 2 ;;
esac
