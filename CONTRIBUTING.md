# Contributing to XAE OS

Thank you for your interest in contributing to XAE OS! This is an educational operating system kernel project designed to teach OS development concepts.

## How to Contribute

### Reporting Issues
- Check if the issue already exists
- Provide detailed information:
  - Steps to reproduce
  - Expected vs actual behavior
  - Build environment (OS, compiler versions)
  - Error messages or screenshots

### Suggesting Features
- Open an issue describing the feature
- Explain why it would be useful for learning
- Provide example use cases

### Submitting Code

1. **Fork the repository**
2. **Create a feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Follow the code style**
   - Add comments explaining WHAT, WHY, and HOW
   - Keep functions small and focused
   - Use descriptive variable names

4. **Test your changes**
   ```bash
   make clean
   make
   make run
   ```

5. **Commit with clear messages**
   ```bash
   git commit -m "Add keyboard input driver with scan code mapping"
   ```

6. **Push and create a pull request**
   ```bash
   git push origin feature/your-feature-name
   ```

## Code Style Guidelines

### C Code
- Use 4 spaces for indentation (no tabs)
- Add function comments explaining purpose
- Keep lines under 80 characters when possible
- Use meaningful variable names

Example:
```c
/*
 * function_name() - Brief description
 * 
 * WHAT: Detailed explanation of what it does
 * WHY: Why this function is necessary
 * HOW: How it accomplishes its task
 */
void function_name(void) {
    // Implementation
}
```

### Assembly Code
- Comment each section explaining purpose
- Use consistent label naming
- Align instructions for readability

Example:
```asm
; Brief description of section
; WHY: Explanation
label:
    mov eax, ebx    ; Explain what this does
    ret
```

## Areas for Contribution

### Beginner-Friendly
- Documentation improvements
- Code comments and explanations
- Bug fixes
- Tutorial examples

### Intermediate
- New device drivers (keyboard, timer, etc.)
- Filesystem enhancements
- Memory management improvements
- Error handling

### Advanced
- Interrupt handling (IDT)
- Process management
- Virtual memory
- Disk I/O drivers

## Documentation

When adding features, please update:
- Code comments (inline documentation)
- README.md if it changes usage
- TUTORIALS.md if you add a tutorial
- DOCUMENTATION.md for technical details

## Questions?

- Open an issue for questions
- Check existing documentation first
- Be specific about what you're trying to accomplish

## Code of Conduct

- Be respectful and constructive
- Focus on learning and education
- Help others understand concepts
- Keep discussions technical and on-topic

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

---

**Thank you for helping make XAE OS a better learning resource!**
