package com.lox;

import java.util.HashMap;
import java.util.Map;

class Environment {

    final Environment enclosing;
    private final Map<String, Object> values = new HashMap<>();

    Environment() {
        enclosing = null;
    }

    Environment(Environment enclosing) {
        this.enclosing = enclosing;
    }

    // I promise this horrendous formatting is not my fault...
    Object get(Token name) {
        if (
            values.containsKey(name.lexeme) && values.get(name.lexeme) != null
        ) {
            return values.get(name.lexeme);
        } else if (
            values.containsKey(name.lexeme) && values.get(name.lexeme) == null
        ) {
            throw new RuntimeError(
                name,
                "Cannot access uninitialized variable '" + name.lexeme + "'."
            );
        }

        if (enclosing != null) return enclosing.get(name);

        throw new RuntimeError(
            name,
            "Undefined variable '" + name.lexeme + "'."
        );
    }

    void assign(Token name, Object value) {
        if (values.containsKey(name.lexeme)) {
            values.put(name.lexeme, value);
            return;
        }

        if (enclosing != null) {
            enclosing.assign(name, value);
            return;
        }

        throw new RuntimeError(
            name,
            "Undefined variable '" + name.lexeme + "'."
        );
    }

    void define(String name, Object value) {
        values.put(name, value);
    }
}
