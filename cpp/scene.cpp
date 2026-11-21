#include "scene.hpp"

#include <algorithm>
#include <iostream>
#include <utility>

Node::Node(std::string new_name, Rect new_rect)
    : name(std::move(new_name)), rect(new_rect) {}

Node::~Node() { std::cout << "DROP " << name << '\n'; }

Editor::Editor() : root_(std::make_shared<Node>("root", Rect{0, 0, 0, 0})) {}

NodeRef Editor::find_in_subtree(const NodeRef& node, const std::string& name) {
    if (!node) return nullptr;
    if (node->name == name) return node;
    for (const auto& child : node->children) {
        if (auto found = find_in_subtree(child, name)) return found;
    }
    return nullptr;
}

NodeRef Editor::find_in_scene(const std::string& name) const {
    return find_in_subtree(root_, name);
}

NodeRef Editor::find_in_clipboard(const std::string& name) const {
    return find_in_subtree(clipboard_, name);
}

NodeRef Editor::parent_of(const NodeRef& node) {
    return node ? node->parent.lock() : nullptr;
}

void Editor::attach(const NodeRef& parent, const NodeRef& child) {
    child->parent = parent;                // weak reference; no cycle
    parent->children.push_back(child);    // strong reference
}

void Editor::print_tree(std::ostream& out) const {
    auto visit = [&](auto&& self, const NodeRef& node, int depth) -> void {
        out << std::string(static_cast<size_t>(depth) * 2, ' ')
            << node->name << " (" << node->rect.x << ',' << node->rect.y
            << " " << node->rect.width << 'x' << node->rect.height << ")\n";
        for (const auto& child : node->children) self(self, child, depth + 1);
    };
    visit(visit, root_, 0);
}

// --------------------- STUDENT IMPLEMENTATIONS -----------------------

bool Editor::add(const std::string& parent, const std::string& name, Rect rect) {
    // Reject bad dimensions and duplicate names (including clipboard).
    if (rect.width <= 0 || rect.height <= 0) return false;
    if (find_in_scene(name) || find_in_clipboard(name)) return false;

    // Find the parent, construct a shared Node, and attach(parent, child).
    auto parent_node = find_in_scene(parent);
    if(!parent_node) return false;

    auto child_node = std::make_shared<Node>(name, rect);
    attach(parent_node, child_node); 
    return true;
}

bool Editor::move_node(const std::string& name, int dx, int dy) {
    // TODO 1: Update only the node's LOCAL x and y.
    auto node = find_in_scene(name);
    if (!node || node == root_) return false;
    node->rect.x += dx;
    node->rect.y += dy;
    return true;
}

Point Editor::world_position_of(const NodeRef& node) {
    // TODO 1: Start at node, accumulate local x/y, repeatedly use parent_of().
    Point world_position{node->rect.x, node->rect.y};
    auto current_node = node;
    while (auto parent_node = parent_of(current_node)) {
        world_position.x += parent_node->rect.x;
        world_position.y += parent_node->rect.y;
        current_node = parent_node;
    }
    // parent_of() already performs weak_ptr::lock(); study it in Part 3.2.
    return world_position;
}

std::optional<Point> Editor::world_position(const std::string& name) const {
    // TODO 1: Find the node, return nullopt if absent, otherwise world_position_of.
    auto node = find_in_scene(name);
    if (!node) return std::nullopt;
    return world_position_of(node);
}

bool Editor::overlaps(const Node& a, Point wa, const Node& b, Point wb) {
    // TODO 1: Pure geometry helper with TWO const Node& parameters.
    if (a.rect.width <= 0 || a.rect.height <= 0 ||
    b.rect.width <= 0 || b.rect.height <= 0) return false;
    
    int ax1 = wa.x;
    int ay1 = wa.y;
    int ax2 = ax1 + a.rect.width;
    int ay2 = ay1 + a.rect.height;
    int bx1 = wb.x;
    int by1 = wb.y;
    int bx2 = bx1 + b.rect.width;
    int by2 = by1 + b.rect.height;

    return ax1 < bx2 && ax2 > bx1 && ay1 < by2 && ay2 > by1;
    // Positive-area overlap only; touching edges are not intersection.
}

std::optional<bool> Editor::intersects(const std::string& a, const std::string& b) const {
    // TODO 1: Find TWO nodes and calculate their world positions.
    auto left = find_in_scene(a);
    auto right = find_in_scene(b);
    if (!left || !right) return std::nullopt;
    auto wa = world_position_of(left);
    auto wb = world_position_of(right);
    // Call overlaps(*left, wa, *right, wb) through read-only references.
    return overlaps(*left, wa, *right, wb);
}

void Editor::swap_local(Node& a, Node& b) {
    // TODO 2: Swap local x/y through TWO mutable C++ references.
    std::swap(a.rect.x, b.rect.x);
    std::swap(a.rect.y, b.rect.y);
}

bool Editor::swap_positions(const std::string& a, const std::string& b) {
    // TODO 2: Obtain two nodes; reject identical nodes and root.
    // Call swap_local(*left, *right) through mutable references.
    auto left = find_in_scene(a);
    auto right = find_in_scene(b);
    if (!left || !right || left == right || left == root_ || right == root_) return false;
    swap_local(*left, *right);
    return true;
}

bool Editor::detach_from_parent(const NodeRef& node) {
    // TODO 2: Use parent_of(). Reject root (which has no parent).
    auto parent_node = parent_of(node);
    if (!parent_node) return false;
    // Remove the node's owning reference from its parent's children vector.
    // Reset the node's weak parent link. The parameter keeps node alive here.
    auto& siblings = parent_node->children;

    siblings.erase(std::remove(siblings.begin(), siblings.end(), node), siblings.end());

    node->parent.reset();
    return true;
}

bool Editor::erase(const std::string& name) {
    // Find in scene, reject root, then detach_from_parent().
    auto node = find_in_scene(name);
    if (!node || node == root_) return false;
    return detach_from_parent(node);
}

NodeRef Editor::selected_node() const {
    // TODO 3.1: Return a strong pointer to the selection (or nullptr).
    // TODO 3.2: selected_ becomes optional<NodeWeak>; lock() it here.
    if (!selected_) return nullptr;
    return selected_->lock();
    
}

bool Editor::select(const std::string& name) {
    // TODO 3.1: Find only in SCENE; store the strong NodeRef in selected_.
    // TODO 3.2: Store a weak pointer instead. Don't retain an owning reference.
    auto node = find_in_scene(name);
    if (!node) return false;
    selected_ = node;
    return true;
}

std::string Editor::info() const {
    // TODO 3.1/3.2:
    // - If selected_ is empty: "NO_SELECTION"
    // - If selected_node() fails: "EXPIRED"
    // - Otherwise: "SELECTED <name>" (do not find it again by name).
    if (!selected_) return "NO_SELECTION";
    auto node = selected_node();
    if (!node) return "EXPIRED";
    return "SELECTED " + node->name;
}

bool Editor::delete_selected() {
    // TODO 3.1/3.2: Obtain a local strong reference using selected_node().
    // Reject root or detached nodes. Detach, then output
    // "AFTER_DETACH <name>" while the local strong reference is still alive.
    // Leave selected_ unchanged so Part 3.1 and 3.2 have different outcomes.
    auto node = selected_node();
    if(!node || node == root_) return false;
    if(!detach_from_parent(node)) return false;
    std::cout << "AFTER_DETACH " << node->name << std::endl;
    return true;
}

bool Editor::cut() {
    // TODO 3.1/3.2: selected_node(), then detach from scene.
    // Move/assign strong ownership to clipboard_ BEFORE releasing other owners.
    // Selection remains unchanged. Replacing clipboard drops its old contents.
    auto node = selected_node();
    if(!node || node == root_) return false;
    if(!detach_from_parent(node)) return false;
    clipboard_ = node;
    return true;
}

bool Editor::paste(const std::string& new_parent) {
    // TODO 3.1/3.2: Attach clipboard root under an existing scene parent.
    // Afterwards, clear clipboard_. Names remain unique because add() checks it.
    auto parent = find_in_scene(new_parent);
    if(!parent) return false;
    if(!clipboard_) return false;
    attach(parent, clipboard_);
    clipboard_.reset();
    return true;
}

void Editor::unselect() { selected_.reset(); } // Supplied: works in both stages.

void Editor::clear_clipboard() { clipboard_.reset(); } // Supplied.
