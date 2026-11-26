# Project 3 Notes — Ownership and lifetime

## Ownership relationships

| Relationship or operation | C++: pointer/operation and ownership effect | Rust: pointer/operation and ownership effect | Observation from a command or code example |
|---|---|---|---|
| Parent -> child | A shared pointer of type node strongly owns each child. |  A `Rc<RefCell<Node>>>` strongly owns each child. | `attach()` pushes a strong pointer or cloned `Rc` into the parent's children (inside a vector). |
| Child -> parent | Uses a weak pointer so the child doesn't own the parent and no cycles are created. | Uses an `Option<Weak<RefCell<Node>>>` so the child doesn't own the parent and no cycles are created. | `parent_of()` uses `weak_ptr::lock()` or `Weak::upgrade()` when an operation needs temporary parent ownership. |
| Tree lookup result | Lookup returns a temporary shared pointer. | Lookup clones the matching `Rc<RefCell<Node>>` and returns it. | `find_in_subtree()` uses a local strong reference to keep a found node alive during lookup and its immediate use, then releases ownership when it goes out of scope. |
| Mutating two different nodes in `swap` | `swap_local()` uses `const` variables to swap each mutable node while `swap_positions()` searches for the node in the tree and rejects the root and identical parameters | `swap_local()` uses `let` variables to swap each mutable node while `swap_positions()` searches for the node in the tree and rejects the root and identical parameters | `swap_positions()` finds the nodes in the tree and calls `swap_local()` after validation checks; Rust also panics if two active mutable borrows accidentally refer to the same `RefCell`. |
| Selection, Part 3.1 | `selected_` is represented as a shared pointer (strong node ownership). | `selected` is a strong `Rc<RefCell<Node>>` (strong node ownership). | Erasing a selected node using `erase()` removes it from the scene (GUI), but a strong selection keeps it alive; `info` can still return `SELECTED <name>`. |
| Selection, Part 3.2 | `selected_` is represented as a weak pointer (weak ownership). | `selected` is `Option<Weak<RefCell<Node>>>` (weak ownership). | `selected_node()` uses `lock()` or `upgrade()` to promote the weak reference only when the node is still owned elsewhere. `info` returns `EXPIRED` after it disappears. |
| Clipboard | `clipboard_` uses shared pointers that strongly owns the detached clipboard root. | `clipboard` is `Option<Rc<RefCell<Node>>>` that strongly owns the detached clipboard root. | Replacing or clearing the clipboard releases that strong ownership. `paste()` reattaches the root beneath a scene parent and then clears it from the clipboard. |
| `delete-selected`: temporary strong owner | `selected_node()` returns a local strong shared pointer during the operation. | `selected_node()` upgrades the weak selection into a local strong `Rc`. | The temporary owner keeps the selected node alive after `detach_from_parent()` removes the parent's owning reference, including while `AFTER_DETACH <name>` is printed. |

## Traces: strong versus weak selection

**Trace A (strong, Part 3.1).** Commands, key output (`DROP` lines), and explanation:

```text  
add root panel 0 0 80 80
add panel button 4 4 12 12
select button
erase button
info -> SELECTED button
unselect -> DROP button, OK
info -> NO_SELECTION
select panel
cut
tree
info -> SELECTED panel
clear-clipboard
info -> SELECTED panel
unselect -> DROP panel, OK
quit DROP root

Analysis:
Strong selection keeps the erased button alive. Only after unselect does the last owner disappear, which means that the node is kept alive with a strong pointer after erase is called.
```

**Trace B (weak, Part 3.2).** Commands, key output (`DROP` lines), and explanation:

```text
add root panel 0 0 80 80
add panel button 4 4 12 12
select button
delete-selected -> AFTER_DETACH button, DROP button, OK
info -> EXPIRED
unselect
info -> NO_SELECTION
select panel
cut
info -> SELECTED panel
clear-clipboard -> DROP panel, OK
info -> EXPIRED
unselect
quit -> DROP root

Analysis:
Weak selection cannot keep nodes alive, but the clipboard's strong ownership keeps panel alive until clear-clipboard. Calling delete-selection releases the button and is able to perform additional operations like printing the name.
```

## Prompts for discussion

### Prompt 1 — Read-only traversal and single ownership changes

C++: A shared pointer is created and is owned by the parent after the attach() method is called. In world_position, a shared pointer and a Point object is created. For each parent, it accumulates the x and y positions of the Point using the values in the parent's x and y fields.

Rust: A Rc<RefCell<T>> is created with new() and is owned by the parent after calling Self::attach(). An immutable borrow guard is created using the node passed so that the function can read it's coordinates. It then creates a Point object and initializes it to those values. For each parent, it accumulates the x and y positions using the values in the parent's x and y fields so a new node added to the tree can use the values calculated by the function.

- Both return a strong pointer held by a local variable.
- Attach adds another strong owner in the parent's children. 
- When add returns, the local owner goes away.

### Prompt 2 — Mutable aliasing and removal

The read-only aliases are acceptable because immutable access doesn't allow modifications to the nodes while they are active. RefCell allows multiple immutable borrows to exist simultaneously, but only one mutable borrow at a time. This is because it helps ensure there are no other ways to modify the borrowed item than the intended mutable alias, thereby decreasing complexity.

Calling erase removes the parent's strong child reference. When erase's temporary strong references go out of scope, the node gets freed/dropped if it isn't owned by the clipboard or selection (or anything else). A local strong reference can be used to keep a node alive through its detachment and any work done immediately after the fact. It also ensures any descendants of the freed node are also released with it.

### Prompt 3.1 — Strong selection and clipboard

-> means owns

Before erase: 
- root -> panel -> button
- Selected -> button

After erase, root no longer owns the node, but strong
selection still owns it.
- root -> panel
- Selected -> button

After cut, the panel gets detached from the root while clipboard becomes a strong owner. Selection remains an owner.
- Selected -> panel
- Clipboard -> panel

When the clipboard is cleared, it releases ownership, but the node stays alive because it is still selected.
- Selected -> panel

It is only after a node is then unselected that nothing is keeping it alive anymore and is subsequently freed.

Since the strong selection stores the node independent from the tree, info still has what it needs to print "SELECTED <name>" after erase.

Allowing nodes to exist beyond detachment and the GUI's representation can allow for users to undo it, but the GUI is displaying a version of the environment that isn't truly reflecting the state of the program. We also have tools like the clipboard/selection to provide user-controlled strong ownership of a node if they want to keep it alive longer.

### Prompt 3.2 — Weak selection and promotion

Why does a weak selection need to be promoted
to strong before use, even in a single-threaded program? In delete-selected,
what precisely keeps the node alive after its parent removes it? Compare the lifetime guarantees and borrowing guarantees of shared_ptr and
Rc<RefCell<T>>.

Weak pointers are helpful for determing whether an object exists or whether it can be upgraded, but it doesn't:
- Own the node it points to (parent)
- The node may not be upgradable after detachment and release.

When we need to use a node to complete a particular task, the only way to keep a node alive is to have a strong reference pointing to it. By promoting it, the node stays alive to perform other operations we may intend to use as cleanup.

In delete-selected, a strong reference that points to the weak selection is created and detached from the tree if there is at least one strong reference already pointing to it (parent in this case). If there isn't a strong reference pointing to the node in question, it fails to upgrade and returns verifying the node doesn't exist. Since the upgraded pointer is created/used inside of a function call, the selected node is kept alive for the duration of that scope so it can perform operations like reading the name and printing "AFTER_DETACH".

Lifetime guarentees:

Both implementations keep objects alive while 1+ strong pointers exist and destroys the object once the count reaches 0. They also have weak pointers that don't own the object they refer to, but can promote with lock or upgrade when needed.

C++ tracks strong reference count but doesn't prevent unsafe mutable aliasing while Rust checks for borrowing rules at runtime using RefCell (multiple read-only but only 1 mutable alias) and uses Rc to control shared lifetime.

Even in a single-threaded program, earlier operations can remove the last strong Rc to an object. Preventing simultaneous access from enforcing its borrowing rules doesn't prevent ownership from changing or previous operations from erasing a node or replacing the clipboard.

## Additional finding or question

Especially during our time in lecture covering the different ways C++ and Rust handle the behavior outlined in this project, their implementations are very similar. The only major difference that I noticed is that Rust has additional rules
for how borrowing works (and its single-threaded), but understanding how the graph and GUI is supposed to work from a conceptual perspective made me realize that my implementations in both languages basically do the same thing. The tools used for ownership use different syntax but perform similarly to each other.
