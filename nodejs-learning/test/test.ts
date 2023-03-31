import request from 'supertest'
import app from '../src/index'

const mdb = require('knex-mariadb');
const knex = require('knex')({
    client: mdb,
    connection: {
        host : '127.0.0.1',
        port : 3306,
        user : 'root',
        password : '0000',
        database : 'nodejs'
      }
  });
  
test('read username from database and it should be merrill', async () => {
    const users = await knex.select('nickname')
        .from('user')

        expect(users[0].nickname).toBe('merrill')
});