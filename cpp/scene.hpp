#pragma once

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

struct Point {
    int x = 0;
    int y = 0;
};

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct Node;
using NodeRef = std::shared_ptr<Node>;
using NodeWeak = std::weak_ptr<Node>;

struct Node {
    std::string name;
    Rect rect;
    NodeWeak parent;                // Scaffold: a child does NOT own its parent.
    std::vector<NodeRef> children;  // Parents own their children.

    Node(std::string name, Rect rect);
    ~Node();                        // Prints DROP <name> for lifetime experiments.
};

class Editor {
public:
    Editor();

    // Part 1
    bool add(const std::string& parent, const std::string& name, Rect rect);
    bool move_node(const std::string& name, int dx, int dy);
    std::optional<Point> world_position(const std::string& name) const;
    std::optional<bool> intersects(const std::string& a, const std::string& b) const;

    // Part 2
    bool swap_positions(const std::string& a, const std::string& b);
    bool erase(const std::string& name);

    // Part 3.1: owning selection and clipboard. Part 3.2: weak selection.
    bool select(const std::string& name);
    std::string info() const; // SELECTED <name> | NO_SELECTION | EXPIRED
    bool delete_selected();
    bool cut();
    bool paste(const std::string& new_parent);
    void unselect();
    void clear_clipboard();

    // Supplied helpers and visualization.
    void print_tree(std::ostream& out) const;

private:
    NodeRef root_;

    // STARTING DESIGN FOR PART 3.1: selection keeps its node alive.
    // PART 3.2: change NodeRef to NodeWeak, then adjust selected_node().
    std::optional<NodeWeak> selected_;
    NodeRef clipboard_;

    // Already implemented: looking up a node temporarily takes a strong reference.
    NodeRef find_in_scene(const std::string& name) const;
    NodeRef find_in_clipboard(const std::string& name) const;
    static NodeRef find_in_subtree(const NodeRef& node, const std::string& name);
    static NodeRef parent_of(const NodeRef& node); // Upgrade weak parent for one operation.
    static void attach(const NodeRef& parent, const NodeRef& child);

    // Students implement these helpers where marked.
    static Point world_position_of(const NodeRef& node); // Part 1
    static bool overlaps(const Node& a, Point wa, const Node& b, Point wb); // Part 1
    static void swap_local(Node& a, Node& b); // Part 2
    static bool detach_from_parent(const NodeRef& node); // Part 2
    NodeRef selected_node() const;                       // Parts 3.1, 3.2
};
