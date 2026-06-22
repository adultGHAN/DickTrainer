# Configuration Reference

## Random Mode

In random mode, **customer**, **size**, and **motion** are each determined randomly.

### Distribution Types

There are two distribution types available: **Normal** and **Uniform**.

#### Normal Distribution

- Activated when `dist` is set to `"normal"`
- A single value is sampled from a normal distribution using `mean` and `std`
- The `level` in `data` closest to the sampled value is selected
- If multiple levels are equally close, one is chosen via uniform distribution

#### Uniform Distribution

- Activated when `dist` is set to `"uniform"`
- A single value is sampled from a uniform distribution over the range `[mean - std, mean + std)`
- The `level` in `data` closest to the sampled value is selected
- If multiple levels are equally close, one is chosen via uniform distribution

---

### Weights

Each **customer** entry may define `size_admend` and `motion_admend` fields.
These values are added to the randomly sampled number before the closest level is selected.

---

### Motion

#### Selection
Motion type is chosen uniformly at random from all available motion types.

#### Repetition
A motion repeats for the number of **courses** defined in the customer.
`warmup` and `cum` do not count toward the course total.

#### Process
```
warmup → motion (× course count) → cum
```

---

### Act

#### Selection
Each motion has multiple levels. A level is selected according to the configured distribution.

#### Repetition
An act repeats for the number of `reps` defined in the selected level.

#### Process
```
preInterval → act (× reps) → postInterval
```

#### Behavior

| Field | Description |
|---|---|
| `pos` | Target depth position. **1** = shallowest, **5** = deepest |
| `move time` | Duration to move to the target `pos` |
| `wait time` | Duration to hold at the target `pos` |
| beep | A beep sound plays at the start of each action |

---

### Repetition Hierarchy

```
Customers (× customer count)
  └─ Courses (× course count per customer)
       └─ Acts (× reps per motion level)
```

---

## Sequence Mode

> Documentation in progress.

---

[← Back to README](../README.md)
