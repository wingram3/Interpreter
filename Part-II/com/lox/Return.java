package com.lox;

class Return extends RuntimeException {

    final Object value;

    Return(Object value) {
        super(null, null, false, false); // Disable unneeded JVM stuff.
        this.value = value;
    }
}
