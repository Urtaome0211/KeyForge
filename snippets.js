// 键锋 KEYFORGE —— 内置练习代码库（window.SNIPPETS，由 app.js 读取）
// 每段的结构：
//   id    全局唯一标识（同时用作历史最佳成绩的 localStorage 存储键）
//   lang  语言分类，必须与 app.js 的 CATEGORIES 对应（js/py/java/c/go/html/css/sql/other）
//   title 下拉框里显示的标题
//   code  要练习的代码原文
// 新增片段注意事项：
//   1. code 只使用可打印 ASCII + 换行；中文等字符无法用键盘直接打出，不适合练习
//   2. 避免反引号与 ${，模板字符串会提前解析导致代码损坏
//   3. 代码里的反斜杠要写成 \\（例如 C 的 "\n" 要写成 "\\n"）
//   4. 缩进统一用空格（不要用 Tab，Tab 会映射成练习里的制表符）

window.SNIPPETS = [
  // ---- JavaScript：语言特性 ----
  {
    id: 'js-fib',
    lang: 'js',
    title: '递归求斐波那契',
    code:
`function fib(n) {
  if (n < 2) return n;
  return fib(n - 1) + fib(n - 2);
}

const seq = [];
for (let i = 0; i < 10; i++) {
  seq.push(fib(i));
}
console.log(seq.join(', '));`
  },
  {
    id: 'js-debounce',
    lang: 'js',
    title: '防抖函数',
    code:
`function debounce(fn, wait) {
  let timer = null;
  return function (...args) {
    clearTimeout(timer);
    timer = setTimeout(() => fn(...args), wait);
  };
}

const onResize = debounce(() => {
  console.log('resized');
}, 300);`
  },
  {
    id: 'js-array',
    lang: 'js',
    title: '数组映射与过滤',
    code:
`const nums = [1, 2, 3, 4, 5, 6];
const squares = nums.map(n => n * n);
const evens = nums.filter(n => n % 2 === 0);
const total = nums.reduce((a, b) => a + b, 0);
console.log(squares, evens, total);`
  },
  {
    id: 'js-fetch',
    lang: 'js',
    title: 'async / await 请求',
    code:
`async function getUser(id) {
  const res = await fetch('/api/user/' + id);
  if (!res.ok) throw new Error('not found');
  const data = await res.json();
  return data;
}

getUser(42).then(console.log);`
  },
  {
    id: 'js-class',
    lang: 'js',
    title: '计数器类',
    code:
`class Counter {
  constructor() {
    this.count = 0;
  }
  up() {
    this.count += 1;
    return this;
  }
}

const c = new Counter();
c.up().up().up();
console.log(c.count); // 3`
  },
  {
    id: 'js-regex',
    lang: 'js',
    title: '正则校验邮箱',
    code:
`const email = 'dev@example.com';
const rule = /^[a-z0-9._-]+@[a-z0-9.-]+$/;
const ok = rule.test(email);

if (!ok) {
  console.warn('invalid email: ' + email);
}`
  },

  // ---- JavaScript：排序 / 查找 / 图遍历 / 链表 ----
  {
    id: 'js-bubble',
    lang: 'js',
    title: '冒泡排序',
    code:
`function bubbleSort(arr) {
  const n = arr.length;
  for (let i = 0; i < n - 1; i++) {
    for (let j = 0; j < n - 1 - i; j++) {
      if (arr[j] > arr[j + 1]) {
        const tmp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = tmp;
      }
    }
  }
  return arr;
}

console.log(bubbleSort([5, 3, 8, 1, 2]));`
  },
  {
    id: 'js-binary',
    lang: 'js',
    title: '二分查找',
    code:
`function binarySearch(arr, target) {
  let lo = 0;
  let hi = arr.length - 1;
  while (lo <= hi) {
    const mid = lo + ((hi - lo) >> 1);
    if (arr[mid] === target) return mid;
    if (arr[mid] < target) lo = mid + 1;
    else hi = mid - 1;
  }
  return -1;
}

console.log(binarySearch([1, 3, 5, 7, 9], 7));`
  },
  {
    id: 'js-linkedlist',
    lang: 'js',
    title: '单链表',
    code:
`class Node {
  constructor(value) {
    this.value = value;
    this.next = null;
  }
}

class LinkedList {
  constructor() {
    this.head = null;
  }
  add(value) {
    const node = new Node(value);
    if (!this.head) {
      this.head = node;
      return;
    }
    let cur = this.head;
    while (cur.next) cur = cur.next;
    cur.next = node;
  }
  print() {
    let cur = this.head;
    while (cur) {
      console.log(cur.value);
      cur = cur.next;
    }
  }
}`
  },
  {
    id: 'js-dfs',
    lang: 'js',
    title: '深度优先搜索 DFS',
    code:
`const graph = {
  A: ['B', 'C'],
  B: ['D', 'E'],
  C: ['F'],
  D: [],
  E: [],
  F: []
};

function dfs(node, visited) {
  if (!visited.has(node)) {
    console.log(node);
    visited.add(node);
    graph[node].forEach(n => dfs(n, visited));
  }
}

dfs('A', new Set());`
  },

  // ---- Python：经典算法与数据结构 ----
  {
    id: 'py-fizzbuzz',
    lang: 'py',
    title: 'FizzBuzz',
    code:
`def fizzbuzz(n):
    if n % 15 == 0:
        return 'FizzBuzz'
    if n % 3 == 0:
        return 'Fizz'
    if n % 5 == 0:
        return 'Buzz'
    return str(n)

for i in range(1, 16):
    print(fizzbuzz(i))`
  },
  {
    id: 'py-comprehension',
    lang: 'py',
    title: '列表推导式与字典',
    code:
`prices = [12.5, 9.0, 33.2, 4.5]
hot = [p for p in prices if p > 10]
discount = {p: round(p * 0.8, 2) for p in prices}
print(hot)
print(discount)`
  },
  {
    id: 'py-decorator',
    lang: 'py',
    title: '计时装饰器',
    code:
`import time

def timer(func):
    def wrapper(*args):
        start = time.time()
        result = func(*args)
        cost = time.time() - start
        print('cost:', cost)
        return result
    return wrapper

@timer
def slow(n):
    return sum(range(n))`
  },
  {
    id: 'py-qsort',
    lang: 'py',
    title: '快速排序',
    code:
`def qsort(a):
    if len(a) <= 1:
        return a
    pivot = a[len(a) // 2]
    left = [x for x in a if x < pivot]
    mid = [x for x in a if x == pivot]
    right = [x for x in a if x > pivot]
    return qsort(left) + mid + qsort(right)`
  },

  {
    id: 'py-insertion',
    lang: 'py',
    title: '插入排序',
    code:
`def insertion_sort(a):
    for i in range(1, len(a)):
        key = a[i]
        j = i - 1
        while j >= 0 and a[j] > key:
            a[j + 1] = a[j]
            j -= 1
        a[j + 1] = key
    return a

print(insertion_sort([9, 4, 7, 2, 6]))`
  },
  {
    id: 'py-bfs',
    lang: 'py',
    title: '广度优先搜索 BFS',
    code:
`from collections import deque

graph = {
    'A': ['B', 'C'],
    'B': ['D'],
    'C': ['E', 'F'],
    'D': [],
    'E': [],
    'F': [],
}

def bfs(start):
    visited = set()
    queue = deque([start])
    while queue:
        node = queue.popleft()
        if node not in visited:
            visited.add(node)
            print(node)
            queue.extend(graph[node])

bfs('A')`
  },
  {
    id: 'py-gcd',
    lang: 'py',
    title: '欧几里得求最大公约数',
    code:
`def gcd(a, b):
    while b != 0:
        a, b = b, a % b
    return a

def lcm(a, b):
    return a * b // gcd(a, b)

print(gcd(48, 18))
print(lcm(4, 6))`
  },

  // ---- Java：排序与查找 ----
  {
    id: 'java-merge',
    lang: 'java',
    title: '归并排序',
    code:
`public class MergeSort {
    static void merge(int[] a, int lo, int mid, int hi) {
        int[] tmp = new int[hi - lo + 1];
        int i = lo, j = mid + 1, k = 0;
        while (i <= mid && j <= hi) {
            tmp[k++] = a[i] <= a[j] ? a[i++] : a[j++];
        }
        while (i <= mid) tmp[k++] = a[i++];
        while (j <= hi) tmp[k++] = a[j++];
        for (int t = 0; t < tmp.length; t++) {
            a[lo + t] = tmp[t];
        }
    }

    static void sort(int[] a, int lo, int hi) {
        if (lo >= hi) return;
        int mid = lo + (hi - lo) / 2;
        sort(a, lo, mid);
        sort(a, mid + 1, hi);
        merge(a, lo, mid, hi);
    }

    public static void main(String[] args) {
        int[] a = {5, 2, 9, 1, 7};
        sort(a, 0, a.length - 1);
        System.out.println(java.util.Arrays.toString(a));
    }
}`
  },
  {
    id: 'java-binary',
    lang: 'java',
    title: '二分查找',
    code:
`public class BinarySearch {
    static int search(int[] a, int target) {
        int lo = 0, hi = a.length - 1;
        while (lo <= hi) {
            int mid = lo + (hi - lo) / 2;
            if (a[mid] == target) return mid;
            if (a[mid] < target) lo = mid + 1;
            else hi = mid - 1;
        }
        return -1;
    }

    public static void main(String[] args) {
        int[] a = {2, 4, 6, 8, 10};
        System.out.println(search(a, 8));
    }
}`
  },

  // ---- C：指针操作 + 排序 / 查找 / 栈 ----
  {
    id: 'c-quick',
    lang: 'c',
    title: '快速排序',
    code:
`#include <stdio.h>

void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

int partition(int a[], int lo, int hi) {
    int pivot = a[hi];
    int i = lo - 1;
    for (int j = lo; j < hi; j++) {
        if (a[j] < pivot) {
            i++;
            swap(&a[i], &a[j]);
        }
    }
    swap(&a[i + 1], &a[hi]);
    return i + 1;
}

void quicksort(int a[], int lo, int hi) {
    if (lo < hi) {
        int p = partition(a, lo, hi);
        quicksort(a, lo, p - 1);
        quicksort(a, p + 1, hi);
    }
}

int main(void) {
    int a[] = {4, 8, 2, 7, 1};
    quicksort(a, 0, 4);
    for (int i = 0; i < 5; i++) {
        printf("%d ", a[i]);
    }
    printf("\\n");
    return 0;
}`
  },
  {
    id: 'c-binary',
    lang: 'c',
    title: '二分查找（指针版）',
    code:
`#include <stdio.h>

int binary_search(int a[], int n, int target) {
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] == target) return mid;
        if (a[mid] < target) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

int main(void) {
    int a[] = {1, 3, 5, 7, 9};
    int pos = binary_search(a, 5, 7);
    printf("index = %d\\n", pos);
    return 0;
}`
  },
  {
    id: 'c-stack',
    lang: 'c',
    title: '数组实现栈',
    code:
`#include <stdio.h>

#define MAX 100

int stack[MAX];
int top = -1;

void push(int x) {
    if (top < MAX - 1) {
        stack[++top] = x;
    }
}

int pop(void) {
    if (top >= 0) {
        return stack[top--];
    }
    return -1;
}

int main(void) {
    push(10);
    push(20);
    push(30);
    printf("%d\\n", pop());
    printf("%d\\n", pop());
    return 0;
}`
  },

  // ---- Go：排序与哈希查找 ----
  {
    id: 'go-selection',
    lang: 'go',
    title: '选择排序',
    code:
`package main

import "fmt"

func selectionSort(a []int) []int {
    n := len(a)
    for i := 0; i < n-1; i++ {
        min := i
        for j := i + 1; j < n; j++ {
            if a[j] < a[min] {
                min = j
            }
        }
        a[i], a[min] = a[min], a[i]
    }
    return a
}

func main() {
    nums := []int{8, 3, 6, 1, 9}
    fmt.Println(selectionSort(nums))
}`
  },
  {
    id: 'go-twosum',
    lang: 'go',
    title: '两数之和（哈希查找）',
    code:
`package main

import "fmt"

func twoSum(nums []int, target int) []int {
    seen := make(map[int]int)
    for i, v := range nums {
        if j, ok := seen[target-v]; ok {
            return []int{j, i}
        }
        seen[v] = i
    }
    return nil
}

func main() {
    fmt.Println(twoSum([]int{2, 7, 11, 15}, 9))
}`
  },

  // ---- HTML：页面结构（练习大量 <> 与属性引号） ----
  {
    id: 'html-form',
    lang: 'html',
    title: '登录表单',
    code:
`<form action="/login" method="post">
  <label for="email">Email</label>
  <input id="email" name="email" type="email" required>
  <label for="pass">Password</label>
  <input id="pass" name="pass" type="password" minlength="8">
  <button type="submit">Sign in</button>
</form>`
  },
  {
    id: 'html-page',
    lang: 'html',
    title: 'HTML5 页面骨架',
    code:
`<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Hello World</title>
</head>
<body>
  <h1>Hello, World!</h1>
  <p>This is a minimal HTML5 page.</p>
</body>
</html>`
  },

  // ---- CSS：布局（练习 {} : ; 等高频符号） ----
  {
    id: 'css-flex',
    lang: 'css',
    title: 'Flex 卡片布局',
    code:
`.card {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 16px 24px;
  border: 1px solid #333;
  border-radius: 8px;
  background: #1a1a2e;
}

.card .title {
  font-size: 18px;
  font-weight: 700;
  color: #e0e0e0;
}`
  },
  {
    id: 'css-grid',
    lang: 'css',
    title: 'Grid 响应式布局',
    code:
`.grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 12px;
}

@media (max-width: 768px) {
  .grid {
    grid-template-columns: 1fr;
  }
}`
  },

  // ---- SQL：查询语句 ----
  {
    id: 'sql-join',
    lang: 'sql',
    title: '多表连接查询',
    code:
`SELECT u.name, o.total
FROM users AS u
JOIN orders AS o
  ON o.user_id = u.id
WHERE o.total > 100
ORDER BY o.total DESC;`
  },
  {
    id: 'sql-group',
    lang: 'sql',
    title: '分组统计',
    code:
`SELECT category, COUNT(*) AS cnt
FROM products
WHERE price > 0
GROUP BY category
HAVING cnt >= 3
ORDER BY cnt DESC;`
  },

  // ---- 其他：JSON / 命令行 ----
  {
    id: 'other-json',
    lang: 'other',
    title: 'JSON 配置',
    code:
`{
  "name": "keyforge",
  "version": "1.0.0",
  "scripts": {
    "start": "node index.js"
  },
  "tags": ["typing", "practice"]
}`
  },
  {
    id: 'other-git',
    lang: 'other',
    title: 'Git 与命令行',
    code:
`git init
git add .
git commit -m "feat: add typing trainer"
git push -u origin main
npm install && npm run dev`
  }
];
