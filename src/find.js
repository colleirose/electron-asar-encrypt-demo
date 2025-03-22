function findEntryModule(mainModule, exports) {
    if (!mainModule || !exports) {
        throw new Error("Invalid input");
    }

    const findModule = (start, target) => {
        if (start.exports === target) {
            return start;
        }

        for (let i = 0; i < start.children.length; i++) {
            const module = findModule(start.children[i], target);

            if (module) {
                return module;
            }
        }

        return null;
    };

    return findModule(mainModule, exports);
}
