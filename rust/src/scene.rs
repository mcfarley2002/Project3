use std::cell::RefCell;
use std::rc::{Rc, Weak};

#[derive(Clone, Copy, Debug)]
pub struct Point {
    pub x: i32,
    pub y: i32,
}

#[derive(Clone, Copy, Debug)]
pub struct Rect {
    pub x: i32,
    pub y: i32,
    pub width: i32,
    pub height: i32,
}

pub type NodeRef = Rc<RefCell<Node>>;
pub type NodeWeak = Weak<RefCell<Node>>;

pub struct Node {
    pub name: String,
    pub rect: Rect,
    pub parent: Option<NodeWeak>, // Scaffold: a child does NOT own its parent.
    pub children: Vec<NodeRef>,   // Parents own their children.
}

impl Node {
    pub fn new(name: &str, rect: Rect) -> NodeRef {
        Rc::new(RefCell::new(Node {
            name: name.to_string(),
            rect,
            parent: None,
            children: Vec::new(),
        }))
    }
}

impl Drop for Node {
    fn drop(&mut self) {
        println!("DROP {}", self.name);
    }
}

pub struct Editor {
    root: NodeRef,

    // STARTING DESIGN FOR PART 3.1: selection keeps its node alive.
    // PART 3.2: change NodeRef to NodeWeak, then adjust selected_node().
    selected: Option<NodeRef>,
    clipboard: Option<NodeRef>,
}

impl Editor {
    pub fn new() -> Self {
        Editor {
            root: Node::new("root", Rect { x: 0, y: 0, width: 0, height: 0 }),
            selected: None,
            clipboard: None,
        }
    }

    // ---------------------- SUPPLIED HELPERS ----------------------

    fn find_in_subtree(node: &NodeRef, name: &str) -> Option<NodeRef> {
        let borrowed = node.borrow();
        if borrowed.name == name {
            return Some(Rc::clone(node));
        }
        for child in &borrowed.children {
            if let Some(found) = Self::find_in_subtree(child, name) {
                return Some(found);
            }
        }
        None
    }

    fn find_in_scene(&self, name: &str) -> Option<NodeRef> {
        Self::find_in_subtree(&self.root, name)
    }

    fn find_in_clipboard(&self, name: &str) -> Option<NodeRef> {
        self.clipboard.as_ref().and_then(|c| Self::find_in_subtree(c, name))
    }

    fn parent_of(node: &NodeRef) -> Option<NodeRef> {
        node.borrow().parent.as_ref().and_then(Weak::upgrade)
    }

    fn attach(parent: &NodeRef, child: &NodeRef) {
        child.borrow_mut().parent = Some(Rc::downgrade(parent));
        parent.borrow_mut().children.push(Rc::clone(child));
    }

    pub fn print_tree(&self) {
        fn visit(node: &NodeRef, depth: usize) {
            let borrowed = node.borrow();
            println!(
                "{}{} ({},{} {}x{})",
                "  ".repeat(depth),
                borrowed.name,
                borrowed.rect.x,
                borrowed.rect.y,
                borrowed.rect.width,
                borrowed.rect.height
            );
            for child in &borrowed.children {
                visit(child, depth + 1);
            }
        }
        visit(&self.root, 0);
    }

    // ------------------- STUDENT IMPLEMENTATIONS ------------------

    pub fn add(&mut self, parent: &str, name: &str, rect: Rect) -> bool {
        // TODO 1: Reject bad dimensions and duplicate names (including clipboard).
        if rect.width <= 0 || rect.height <= 0 {
            return false;
        }
        if self.find_in_scene(name).is_some() || self.find_in_clipboard(name).is_some() {
            return false;
        }
        // Find parent, construct Node::new(), and Self::attach(parent, child).
        if let Some(parent_node) = self.find_in_scene(parent) {
            let child_node = Node::new(name, rect);
            Self::attach(&parent_node, &child_node);
            return true;
        }
        false
    }

    pub fn move_node(&mut self, name: &str, dx: i32, dy: i32) -> bool {
        // TODO 1: Update only the node's LOCAL x and y.
        if let Some(node) = self.find_in_scene(name) {
            if Rc::ptr_eq(&node, &self.root) {
                return false;
            }
            let mut borrowed = node.borrow_mut();
            borrowed.rect.x += dx;
            borrowed.rect.y += dy;
            return true;
        }
        false
    }

    fn world_position_of(node: &NodeRef) -> Point {
        // TODO 1: Accumulate local positions, repeatedly use Self::parent_of().
        let mut world_position = {
            let borrowed = node.borrow();
            Point { x: borrowed.rect.x, y: borrowed.rect.y }
        };
        let mut current_node = Rc::clone(node);

        while let Some(parent_node) = Self::parent_of(&current_node) {
            world_position.x += parent_node.borrow().rect.x;
            world_position.y += parent_node.borrow().rect.y;
            current_node = parent_node;
        }
        // parent_of() already performs Weak::upgrade(); study it in Part 3.2.
        world_position
    }

    pub fn world_position(&self, name: &str) -> Option<Point> {
        // TODO 1: Find node in scene, or return None.
        self.find_in_scene(name).map(|node| Self::world_position_of(&node))
    }

    fn overlaps(a: &Node, wa: Point, b: &Node, wb: Point) -> bool {
        // TODO 1: Pure geometry helper with TWO shared &Node parameters.
        if a.rect.width <= 0 || a.rect.height <= 0 ||
        b.rect.width <= 0 || b.rect.height <= 0 { return false; }

        let ax1 = wa.x;
        let ay1 = wa.y;
        let ax2 = ax1 + a.rect.width;
        let ay2 = ay1 + a.rect.height;
        let bx1 = wb.x;
        let by1 = wb.y;
        let bx2 = bx1 + b.rect.width;
        let by2 = by1 + b.rect.height;

        ax1 < bx2 && ax2 > bx1 && ay1 < by2 && ay2 > by1
    }

    pub fn intersects(&self, a: &str, b: &str) -> Option<bool> {
        // TODO 1: Find TWO nodes, compute world positions, borrow both
        // immutably, and call overlaps(&left_node, wa, &right_node, wb).
        let left = self.find_in_scene(a);
        let right = self.find_in_scene(b);
        if let (Some(left_node), Some(right_node)) = (left, right) {
            let wa = Self::world_position_of(&left_node);
            let wb = Self::world_position_of(&right_node);
            return Some(Self::overlaps(&left_node.borrow(), wa, &right_node.borrow(), wb));
        }
        None
    }

    fn swap_local(a: &mut Node, b: &mut Node) {
        // TODO 2: Swap local x/y through TWO exclusive mutable borrows.
        let temp_x = a.rect.x;
        let temp_y = a.rect.y;
        a.rect.x = b.rect.x;
        a.rect.y = b.rect.y;
        b.rect.x = temp_x;
        b.rect.y = temp_y;
    }

    pub fn swap_positions(&mut self, a: &str, b: &str) -> bool {
        // TODO 2: Find two nodes, reject identical nodes (Rc::ptr_eq) or root.
        // Obtain two RefMut<Node> values and call swap_local(&mut first, &mut second).
        let left = self.find_in_scene(a);
        let right = self.find_in_scene(b);
        if let (Some(left_node), Some(right_node)) = (left, right) {
            if Rc::ptr_eq(&left_node, &right_node) || Rc::ptr_eq(&left_node, &self.root) || Rc::ptr_eq(&right_node, &self.root) {
                return false;
            }
            let mut left_borrow = left_node.borrow_mut();
            let mut right_borrow = right_node.borrow_mut();
            Self::swap_local(&mut left_borrow, &mut right_borrow);
            return true;
        }
        false
    }

    fn detach_from_parent(node: &NodeRef) -> bool {
        // TODO 2: Use Self::parent_of(). Reject root (no parent).
        // Remove the owning Rc from parent's children vector.
        // Reset node's weak parent. Parameter keeps node alive while detaching.
        if let Some(parent) = Self::parent_of(node) {
            parent.borrow_mut().children.retain(|child| !Rc::ptr_eq(child, node));
            node.borrow_mut().parent = None;
            return true;
        }
        false
    }

    pub fn erase(&mut self, name: &str) -> bool {
        // TODO 2: Find in scene, reject root, then detach_from_parent().
        if let Some(node) = self.find_in_scene(name) {
            if Rc::ptr_eq(&node, &self.root) {
                return false;
            }
            return Self::detach_from_parent(&node);
        }
        false
    }

    fn selected_node(&self) -> Option<NodeRef> {
        // TODO 3.1: Clone the strong selected Rc, if any.
        // TODO 3.2: selected becomes Option<NodeWeak>; upgrade() it here.
        None
    }

    pub fn select(&mut self, name: &str) -> bool {
        // TODO 3.1: Find only in SCENE; selected = Some(strong Rc).
        // TODO 3.2: Store Rc::downgrade(&node) instead.
        let _ = name;
        false
    }

    pub fn info(&self) -> String {
        // TODO 3.1/3.2:
        // - If selected is None: "NO_SELECTION"
        // - If selected_node() fails: "EXPIRED"
        // - Otherwise: "SELECTED <name>" (do not find by name again).
        "TODO info".to_string()
    }

    pub fn delete_selected(&mut self) -> bool {
        // TODO 3.1/3.2: Obtain local strong Rc using selected_node().
        // Reject root/detached nodes. Detach, then print
        // "AFTER_DETACH <name>" while the local Rc is still alive.
        // Leave selected unchanged to contrast 3.1 with 3.2.
        false
    }

    pub fn cut(&mut self) -> bool {
        // TODO 3.1/3.2: selected_node(), then detach from scene.
        // Assign Rc to clipboard BEFORE releasing other owners.
        // Selection stays unchanged. Replacing clipboard drops its old contents.
        false
    }

    pub fn paste(&mut self, new_parent: &str) -> bool {
        // TODO 3.1/3.2: Attach clipboard under an existing scene parent,
        // then set clipboard to None.
        let _ = new_parent;
        false
    }

    pub fn unselect(&mut self) {
        self.selected = None;
    }

    pub fn clear_clipboard(&mut self) {
        self.clipboard = None;
    }
}
