// ============================================================
// 本文件由 tools/gen-snippets.js 自动生成 —— 请勿手工修改！
// 数据来源：网页版 snippets.js 的 window.SNIPPETS（32 段练习代码）
// 重新生成：node tools/gen-snippets.js
// ============================================================
#include "snippetdata.h"

namespace Keyforge {

const QVector<SnippetDef> kSnippets = {
    { QStringLiteral("js-fib"), QStringLiteral("js"), QStringLiteral("递归求斐波那契"),
      QString::fromUtf8(R"KFG(function fib(n) {
  if (n < 2) return n;
  return fib(n - 1) + fib(n - 2);
}

const seq = [];
for (let i = 0; i < 10; i++) {
  seq.push(fib(i));
}
console.log(seq.join(', '));)KFG") },
    { QStringLiteral("js-debounce"), QStringLiteral("js"), QStringLiteral("防抖函数"),
      QString::fromUtf8(R"KFG(function debounce(fn, wait) {
  let timer = null;
  return function (...args) {
    clearTimeout(timer);
    timer = setTimeout(() => fn(...args), wait);
  };
}

const onResize = debounce(() => {
  console.log('resized');
}, 300);)KFG") },
    { QStringLiteral("js-array"), QStringLiteral("js"), QStringLiteral("数组映射与过滤"),
      QString::fromUtf8(R"KFG(const nums = [1, 2, 3, 4, 5, 6];
const squares = nums.map(n => n * n);
const evens = nums.filter(n => n % 2 === 0);
const total = nums.reduce((a, b) => a + b, 0);
console.log(squares, evens, total);)KFG") },
    { QStringLiteral("js-fetch"), QStringLiteral("js"), QStringLiteral("async / await 请求"),
      QString::fromUtf8(R"KFG(async function getUser(id) {
  const res = await fetch('/api/user/' + id);
  if (!res.ok) throw new Error('not found');
  const data = await res.json();
  return data;
}

getUser(42).then(console.log);)KFG") },
    { QStringLiteral("js-class"), QStringLiteral("js"), QStringLiteral("计数器类"),
      QString::fromUtf8(R"KFG(class Counter {
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
console.log(c.count); // 3)KFG") },
    { QStringLiteral("js-regex"), QStringLiteral("js"), QStringLiteral("正则校验邮箱"),
      QString::fromUtf8(R"KFG(const email = 'dev@example.com';
const rule = /^[a-z0-9._-]+@[a-z0-9.-]+$/;
const ok = rule.test(email);

if (!ok) {
  console.warn('invalid email: ' + email);
})KFG") },
    { QStringLiteral("js-bubble"), QStringLiteral("js"), QStringLiteral("冒泡排序"),
      QString::fromUtf8(R"KFG(function bubbleSort(arr) {
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

console.log(bubbleSort([5, 3, 8, 1, 2]));)KFG") },
    { QStringLiteral("js-binary"), QStringLiteral("js"), QStringLiteral("二分查找"),
      QString::fromUtf8(R"KFG(function binarySearch(arr, target) {
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

console.log(binarySearch([1, 3, 5, 7, 9], 7));)KFG") },
    { QStringLiteral("js-linkedlist"), QStringLiteral("js"), QStringLiteral("单链表"),
      QString::fromUtf8(R"KFG(class Node {
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
})KFG") },
    { QStringLiteral("js-dfs"), QStringLiteral("js"), QStringLiteral("深度优先搜索 DFS"),
      QString::fromUtf8(R"KFG(const graph = {
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

dfs('A', new Set());)KFG") },
    { QStringLiteral("py-fizzbuzz"), QStringLiteral("py"), QStringLiteral("FizzBuzz"),
      QString::fromUtf8(R"KFG(def fizzbuzz(n):
    if n % 15 == 0:
        return 'FizzBuzz'
    if n % 3 == 0:
        return 'Fizz'
    if n % 5 == 0:
        return 'Buzz'
    return str(n)

for i in range(1, 16):
    print(fizzbuzz(i)))KFG") },
    { QStringLiteral("py-comprehension"), QStringLiteral("py"), QStringLiteral("列表推导式与字典"),
      QString::fromUtf8(R"KFG(prices = [12.5, 9.0, 33.2, 4.5]
hot = [p for p in prices if p > 10]
discount = {p: round(p * 0.8, 2) for p in prices}
print(hot)
print(discount))KFG") },
    { QStringLiteral("py-decorator"), QStringLiteral("py"), QStringLiteral("计时装饰器"),
      QString::fromUtf8(R"KFG(import time

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
    return sum(range(n)))KFG") },
    { QStringLiteral("py-qsort"), QStringLiteral("py"), QStringLiteral("快速排序"),
      QString::fromUtf8(R"KFG(def qsort(a):
    if len(a) <= 1:
        return a
    pivot = a[len(a) // 2]
    left = [x for x in a if x < pivot]
    mid = [x for x in a if x == pivot]
    right = [x for x in a if x > pivot]
    return qsort(left) + mid + qsort(right))KFG") },
    { QStringLiteral("py-insertion"), QStringLiteral("py"), QStringLiteral("插入排序"),
      QString::fromUtf8(R"KFG(def insertion_sort(a):
    for i in range(1, len(a)):
        key = a[i]
        j = i - 1
        while j >= 0 and a[j] > key:
            a[j + 1] = a[j]
            j -= 1
        a[j + 1] = key
    return a

print(insertion_sort([9, 4, 7, 2, 6])))KFG") },
    { QStringLiteral("py-bfs"), QStringLiteral("py"), QStringLiteral("广度优先搜索 BFS"),
      QString::fromUtf8(R"KFG(from collections import deque

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

bfs('A'))KFG") },
    { QStringLiteral("py-gcd"), QStringLiteral("py"), QStringLiteral("欧几里得求最大公约数"),
      QString::fromUtf8(R"KFG(def gcd(a, b):
    while b != 0:
        a, b = b, a % b
    return a

def lcm(a, b):
    return a * b // gcd(a, b)

print(gcd(48, 18))
print(lcm(4, 6)))KFG") },
    { QStringLiteral("java-merge"), QStringLiteral("java"), QStringLiteral("归并排序"),
      QString::fromUtf8(R"KFG(public class MergeSort {
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
})KFG") },
    { QStringLiteral("java-binary"), QStringLiteral("java"), QStringLiteral("二分查找"),
      QString::fromUtf8(R"KFG(public class BinarySearch {
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
})KFG") },
    { QStringLiteral("c-quick"), QStringLiteral("c"), QStringLiteral("快速排序"),
      QString::fromUtf8(R"KFG(#include <stdio.h>

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
    printf("\n");
    return 0;
})KFG") },
    { QStringLiteral("c-binary"), QStringLiteral("c"), QStringLiteral("二分查找（指针版）"),
      QString::fromUtf8(R"KFG(#include <stdio.h>

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
    printf("index = %d\n", pos);
    return 0;
})KFG") },
    { QStringLiteral("c-stack"), QStringLiteral("c"), QStringLiteral("数组实现栈"),
      QString::fromUtf8(R"KFG(#include <stdio.h>

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
    printf("%d\n", pop());
    printf("%d\n", pop());
    return 0;
})KFG") },
    { QStringLiteral("go-selection"), QStringLiteral("go"), QStringLiteral("选择排序"),
      QString::fromUtf8(R"KFG(package main

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
})KFG") },
    { QStringLiteral("go-twosum"), QStringLiteral("go"), QStringLiteral("两数之和（哈希查找）"),
      QString::fromUtf8(R"KFG(package main

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
})KFG") },
    { QStringLiteral("html-form"), QStringLiteral("html"), QStringLiteral("登录表单"),
      QString::fromUtf8(R"KFG(<form action="/login" method="post">
  <label for="email">Email</label>
  <input id="email" name="email" type="email" required>
  <label for="pass">Password</label>
  <input id="pass" name="pass" type="password" minlength="8">
  <button type="submit">Sign in</button>
</form>)KFG") },
    { QStringLiteral("html-page"), QStringLiteral("html"), QStringLiteral("HTML5 页面骨架"),
      QString::fromUtf8(R"KFG(<!DOCTYPE html>
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
</html>)KFG") },
    { QStringLiteral("css-flex"), QStringLiteral("css"), QStringLiteral("Flex 卡片布局"),
      QString::fromUtf8(R"KFG(.card {
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
})KFG") },
    { QStringLiteral("css-grid"), QStringLiteral("css"), QStringLiteral("Grid 响应式布局"),
      QString::fromUtf8(R"KFG(.grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 12px;
}

@media (max-width: 768px) {
  .grid {
    grid-template-columns: 1fr;
  }
})KFG") },
    { QStringLiteral("sql-join"), QStringLiteral("sql"), QStringLiteral("多表连接查询"),
      QString::fromUtf8(R"KFG(SELECT u.name, o.total
FROM users AS u
JOIN orders AS o
  ON o.user_id = u.id
WHERE o.total > 100
ORDER BY o.total DESC;)KFG") },
    { QStringLiteral("sql-group"), QStringLiteral("sql"), QStringLiteral("分组统计"),
      QString::fromUtf8(R"KFG(SELECT category, COUNT(*) AS cnt
FROM products
WHERE price > 0
GROUP BY category
HAVING cnt >= 3
ORDER BY cnt DESC;)KFG") },
    { QStringLiteral("other-json"), QStringLiteral("other"), QStringLiteral("JSON 配置"),
      QString::fromUtf8(R"KFG({
  "name": "keyforge",
  "version": "1.0.0",
  "scripts": {
    "start": "node index.js"
  },
  "tags": ["typing", "practice"]
})KFG") },
    { QStringLiteral("other-git"), QStringLiteral("other"), QStringLiteral("Git 与命令行"),
      QString::fromUtf8(R"KFG(git init
git add .
git commit -m "feat: add typing trainer"
git push -u origin main
npm install && npm run dev)KFG") },
};

} // namespace Keyforge
