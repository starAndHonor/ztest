快速，低学习成本，适用于中小项目，可拓展的单元测试库。
TestComponent (抽象基类)
    ↑
TestEntity (抽象类，添加基础属性)
    ↑
├── TestCase (叶子节点，执行单个测试方法)
└── TestSuite (容器节点，管理子组件)
        ↑
        └── GroupedTestSuite (支持标签分组的高级套件)
ztestbase:
