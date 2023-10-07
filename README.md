# HTTP Trie Router
C++ program designed and written in C++ to map out http redirect rules with flexibility.

## Syntax
Rules are fed to the program in reverse, this is to ensure consistency and prevent user error.
```json
{
    "rules": {
        "to": [
            "from1",
            "from2"
        ]
    }
}
```

'from' rules can be in any of the following formats:
1. Redirect from host: `www.example.com`
2. Redirect from path (matches any host): `/home`
3. Redirect from specific host and path: `www.example.com/home`
4. Redirect with wildcard: `www.example.com/home/*`

## Features
1. Allows for recursive redirect resolvance.
2. Redirections falling onto wildcard rules will resolve to closest wildcard rule.
3. Smart path construction in wildcard redirects.

# License
WTFPL. Also present in the `main.cpp` file.
