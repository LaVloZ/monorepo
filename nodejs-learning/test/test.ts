import request from 'supertest'
import app from '../src/index'
  
test('adds 1 + 2 to equal 3', async () => {
    const response = await request(app).get('/');
    expect(response.status).toBe(200);
});