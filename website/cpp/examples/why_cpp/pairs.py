# One pass over every pair of 512 LJ atoms, in pure Python.
import time

N, L, rc2 = 512, 8.6177, 2.5**2      # 512 atoms, density 0.8, cutoff 2.5
a = L / 8                            # an 8 x 8 x 8 grid, as on the home page
x = [[(i + 0.5) * a, (j + 0.5) * a, (k + 0.5) * a]
     for i in range(8) for j in range(8) for k in range(8)]

def energy(x):
    e = 0.0
    for i in range(N):
        xi, yi, zi = x[i]
        for j in range(i + 1, N):
            dx = x[j][0] - xi; dy = x[j][1] - yi; dz = x[j][2] - zi
            dx -= L * round(dx / L); dy -= L * round(dy / L); dz -= L * round(dz / L)
            r2 = dx * dx + dy * dy + dz * dz
            if r2 > rc2:
                continue
            sr6 = (1.0 / r2) ** 3
            e += 4.0 * (sr6 * sr6 - sr6)
    return e

best = 1e9
for _ in range(3):
    t = time.perf_counter()
    e = energy(x)
    best = min(best, time.perf_counter() - t)
print(f"python: U = {e:.6f}   {best * 1e3:.1f} ms per pass")
