int c() {
  int i = 3;
  return i * 15;
}

void b() {
  c();
}

void a() {
  b();
  c();
}


int main() {
  a();
  b();

  return c();
}
